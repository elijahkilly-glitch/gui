#!/usr/bin/env python3
"""
build.py  —  x9 external cheat builder
=================================================

Automates the full build pipeline:
  1. Locates MSVC (cl.exe) via vswhere or fallback paths
  2. Syncs all outputs (theme headers, esp/menu dispatchers, shared state)
  3. Validates all required source / header files are present
  4. Compiles everything into a single EXE (or DLL)
  5. Embeds UAC manifest, strips PDB, optionally UPX-compresses
  6. Prints a clean build summary with file sizes and timings

File structure this script knows about:
  x9/
  ├── build.py                   ← this file
  ├── main.cpp
  ├── aimbot.cpp / .h
  ├── esp.cpp                    ← dispatcher (X9_SHARED_IMPL defined here)
  ├── esp_theme_normal.h         ← normal ESP (included by esp.cpp)
  ├── menu.cpp                   ← dispatcher
  ├── menu_theme_industrial.h    ← industrial menu
  ├── menu_theme_original.h      ← original menu
  ├── x9_shared.h                ← shared Spotify/session state globals
  ├── settings.h / settings.cpp
  ├── overlay.cpp / .h
  ├── loader.cpp / .h
  ├── roblox.cpp / .h
  ├── driver.cpp / .h
  ├── console.cpp / .h
  ├── app.manifest
  └── imgui/
        imgui.cpp, imgui_draw.cpp, imgui_tables.cpp,
        imgui_widgets.cpp, imgui_impl_win32.cpp, imgui_impl_dx11.cpp

Usage
-----
  python build.py                  # release EXE
  python build.py --debug          # debug build (console + PDB)
  python build.py --dll            # build as injectable DLL
  python build.py --clean          # wipe build/ and exit
  python build.py --no-sync        # skip output sync step
  python build.py --no-compress    # skip UPX compression
  python build.py --outdir my_out  # custom output directory

Requirements
------------
  - Visual Studio 2019 or 2022 with MSVC C++ toolchain
  - Windows SDK 10+
  - DirectX 11 headers (bundled with Windows SDK)
  - ImGui source in <project>/imgui/
  - Optional: UPX on PATH for output compression
"""

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import List, Optional, Tuple


# ─────────────────────────────────────────────────────────────────
#  Colour output
# ─────────────────────────────────────────────────────────────────
_USE_COLOR = sys.platform != "win32" or os.environ.get("TERM") == "xterm"
try:
    import colorama; colorama.init(); _USE_COLOR = True
except ImportError:
    pass

def _c(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if _USE_COLOR else text

def ok(msg):   print(_c("92",   f"  ✓  {msg}"))
def info(msg): print(_c("96",   f"  →  {msg}"))
def warn(msg): print(_c("93",   f"  ⚠  {msg}"), file=sys.stderr)
def err(msg):  print(_c("91",   f"  ✗  {msg}"), file=sys.stderr)
def head(msg): print(_c("1;97", f"\n{'─'*60}\n  {msg}\n{'─'*60}"))


# ─────────────────────────────────────────────────────────────────
#  Project layout
# ─────────────────────────────────────────────────────────────────

SCRIPT_DIR   = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR
BUILD_DIR    = PROJECT_ROOT / "build"
IMGUI_DIR    = PROJECT_ROOT / "imgui"

# ── .cpp files that get compiled into object files ───────────────
# NOTE: header-only theme files (menu_theme_*.h, esp_theme_*.h,
#       x9_shared.h) are #included by their dispatcher .cpp files
#       so they do NOT appear here.
# NOTE: settings_friendly.cpp and roblox_antiaim_patch.cpp define
#       duplicate symbols — they must NEVER be compiled.
PROJECT_SOURCES: List[str] = [
    "main.cpp",
    "aimbot.cpp",
    "esp.cpp",        # includes x9_shared.h (X9_SHARED_IMPL), esp_theme_normal.h
    "menu.cpp",       # includes menu_theme_industrial.h, menu_theme_original.h
    "overlay.cpp",
    "console.cpp",
    "loader.cpp",
    "roblox.cpp",
    "driver.cpp",
    "settings.cpp",
]

# ── ImGui backend sources ────────────────────────────────────────
IMGUI_SOURCES: List[str] = [
    "imgui/imgui.cpp",
    "imgui/imgui_draw.cpp",
    "imgui/imgui_tables.cpp",
    "imgui/imgui_widgets.cpp",
    "imgui/imgui_impl_win32.cpp",
    "imgui/imgui_impl_dx11.cpp",
]

# ── Headers that must exist before compilation starts ────────────
REQUIRED_HEADERS: List[str] = [
    # Core headers
    "aimbot.h",
    "aimbot_settings_patch.h",  # documents new Settings fields
    "console.h",
    "esp.h",
    "loader.h",
    "menu.h",
    "overlay.h",
    "roblox.h",
    "settings.h",
    "driver.h",
    # New theme system headers
    "x9_shared.h",
    "config.h",
    "config_ui.h",
    "chams.h",
    "esp_theme_normal.h",
    "menu_theme_industrial.h",
    "menu_theme_original.h",
    # ImGui
    "imgui/imgui.h",
    "imgui/imgui_impl_win32.h",
    "imgui/imgui_impl_dx11.h",
]

# ── Output sync map ───────────────────────────────────────────────
# Maps:  outputs/<filename>  →  project/<destination>
# Run after build sessions to copy updated files into the project.
SYNC_OUTPUTS: dict = {
    # ── Dispatchers (compiled .cpp files) ────────────────────────
    "menu.cpp":                 "menu.cpp",
    "esp.cpp":                  "esp.cpp",

    # ── Theme headers (included by dispatchers, not compiled directly) ──
    "menu_theme_industrial.h":  "menu_theme_industrial.h",
    "menu_theme_original.h":    "menu_theme_original.h",
    "esp_theme_normal.h":       "esp_theme_normal.h",

    # ── Shared state (included by esp.cpp and placeholder) ─
    "x9_shared.h":              "x9_shared.h",
    "config.h":                 "config.h",
    "config_ui.h":              "config_ui.h",
    "chams.h":                  "chams.h",

    # ── Other updated sources ─────────────────────────────────────
    "aimbot.cpp":               "aimbot.cpp",
    "aimbot_settings_patch.h":  "aimbot_settings_patch.h",
    "overlay.cpp":              "overlay.cpp",
    "loader.cpp":               "loader.cpp",
    "loader.h":                 "loader.h",
    "main.cpp":                 "main.cpp",
    "driver.cpp":               "driver.cpp",
    "settings.h":               "settings.h",
    "settings.cpp":             "settings.cpp",
}

# ── Per-source extra compile flags ───────────────────────────────
PER_SOURCE_FLAGS: dict = {
    # esp.cpp defines X9_SHARED_IMPL via #define before #include "x9_shared.h"
    # so no extra flag needed — it's handled inside the file itself.
}

DEFAULT_OUT_NAME = "x9"


# ─────────────────────────────────────────────────────────────────
#  MSVC detection
# ─────────────────────────────────────────────────────────────────

VSWHERE_PATHS = [
    r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
    r"C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe",
]

MSVC_FALLBACK_ROOTS = [
    r"C:\Program Files\Microsoft Visual Studio\2022",
    r"C:\Program Files (x86)\Microsoft Visual Studio\2022",
    r"C:\Program Files\Microsoft Visual Studio\2019",
    r"C:\Program Files (x86)\Microsoft Visual Studio\2019",
]


def find_vswhere() -> Optional[Path]:
    for p in VSWHERE_PATHS:
        if Path(p).exists():
            return Path(p)
    return None


def find_msvc_via_vswhere(vswhere: Path) -> Optional[Path]:
    try:
        out = subprocess.check_output([
            str(vswhere), "-latest", "-products", "*",
            "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property", "installationPath",
        ], text=True, stderr=subprocess.DEVNULL).strip()
        if not out:
            return None
        vs_path = Path(out)
        ver_file = vs_path / "VC" / "Auxiliary" / "Build" / "Microsoft.VCToolsVersion.default.txt"
        if ver_file.exists():
            vc_ver   = ver_file.read_text().strip()
            vc_tools = vs_path / "VC" / "Tools" / "MSVC" / vc_ver
        else:
            candidates = list((vs_path / "VC" / "Tools" / "MSVC").glob("*"))
            if not candidates:
                return None
            vc_tools = sorted(candidates)[-1]
        cl = vc_tools / "bin" / "Hostx64" / "x64" / "cl.exe"
        return vc_tools if cl.exists() else None
    except Exception:
        return None


def find_msvc_fallback() -> Optional[Path]:
    for root in MSVC_FALLBACK_ROOTS:
        for cl in Path(root).rglob("cl.exe"):
            if "Hostx64" in str(cl) and "x64" in str(cl):
                return cl.parent.parent.parent.parent
    return None


def find_windows_sdk() -> Tuple[Optional[Path], Optional[str]]:
    sdk_root, sdk_ver = None, None
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                             r"SOFTWARE\Microsoft\Windows Kits\Installed Roots")
        sdk_root = Path(winreg.QueryValueEx(key, "KitsRoot10")[0])
        versions = sorted(
            [d.name for d in (sdk_root / "Include").iterdir()
             if d.is_dir() and d.name.startswith("10.")],
            reverse=True
        )
        if versions:
            sdk_ver = versions[0]
    except Exception:
        pass

    if not sdk_root or not sdk_ver:
        for root in [
            Path(r"C:\Program Files (x86)\Windows Kits\10"),
            Path(r"C:\Program Files\Windows Kits\10"),
        ]:
            if root.exists():
                sdk_root = root
                versions = sorted(
                    [d.name for d in (root / "Include").iterdir()
                     if d.is_dir() and d.name.startswith("10.")],
                    reverse=True
                )
                if versions:
                    sdk_ver = versions[0]
                    break

    return sdk_root, sdk_ver


# ─────────────────────────────────────────────────────────────────
#  Build environment
# ─────────────────────────────────────────────────────────────────

class BuildEnv:
    def __init__(self):
        self.cl:           Optional[Path] = None
        self.link:         Optional[Path] = None
        self.vc_tools:     Optional[Path] = None
        self.sdk_root:     Optional[Path] = None
        self.sdk_ver:      Optional[str]  = None
        self.include_dirs: List[Path]     = []
        self.lib_dirs:     List[Path]     = []
        self.env:          dict           = {}

    def locate(self) -> bool:
        info("Locating MSVC toolchain...")

        vc_tools = None
        vswhere  = find_vswhere()
        if vswhere:
            vc_tools = find_msvc_via_vswhere(vswhere)
            if vc_tools:
                ok(f"Found via vswhere: {vc_tools}")
        if not vc_tools:
            vc_tools = find_msvc_fallback()
            if vc_tools:
                ok(f"Found via fallback: {vc_tools}")
        if not vc_tools:
            err("MSVC not found. Install Visual Studio 2019/2022 "
                "with 'Desktop development with C++'.")
            return False

        self.vc_tools = vc_tools
        self.cl   = vc_tools / "bin" / "Hostx64" / "x64" / "cl.exe"
        self.link = vc_tools / "bin" / "Hostx64" / "x64" / "link.exe"

        info("Locating Windows SDK...")
        sdk_root, sdk_ver = find_windows_sdk()
        if not sdk_root or not sdk_ver:
            err("Windows SDK 10 not found. "
                "Install via Visual Studio Installer.")
            return False
        ok(f"Windows SDK {sdk_ver} at {sdk_root}")
        self.sdk_root = sdk_root
        self.sdk_ver  = sdk_ver

        vc_inc  = vc_tools / "include"
        sdk_inc = sdk_root / "Include" / sdk_ver
        vc_lib  = vc_tools / "lib" / "x64"
        sdk_lib = sdk_root / "Lib"     / sdk_ver

        self.include_dirs = [
            vc_inc,
            sdk_inc / "ucrt",
            sdk_inc / "um",
            sdk_inc / "shared",
            PROJECT_ROOT,       # project root — finds all .h files directly
            IMGUI_DIR,          # imgui/ subdir
        ]
        self.lib_dirs = [
            vc_lib,
            sdk_lib / "ucrt" / "x64",
            sdk_lib / "um"   / "x64",
        ]

        self.env = dict(os.environ)
        self.env["PATH"] = (
            str(self.cl.parent) + os.pathsep + self.env.get("PATH", "")
        )
        return True


# ─────────────────────────────────────────────────────────────────
#  Output sync
# ─────────────────────────────────────────────────────────────────

def sync_outputs(outputs_dir: Path) -> None:
    head("Syncing build outputs → project sources")
    if not outputs_dir.exists():
        warn(f"Outputs directory not found: {outputs_dir}  (skipping sync)")
        return

    synced = skipped = 0
    for out_name, dest_rel in SYNC_OUTPUTS.items():
        src  = outputs_dir / out_name
        dest = PROJECT_ROOT / dest_rel
        if src.exists():
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dest)
            ok(f"Synced  {out_name:40s}  →  {dest_rel}")
            synced += 1
        else:
            warn(f"Output not found, skipping: {out_name}")
            skipped += 1

    info(f"Sync complete: {synced} copied, {skipped} skipped")


# ─────────────────────────────────────────────────────────────────
#  Source validation
# ─────────────────────────────────────────────────────────────────

def validate_sources() -> bool:
    head("Validating source files")
    hard_missing = []
    soft_missing = []

    for hdr in REQUIRED_HEADERS:
        p = PROJECT_ROOT / hdr
        if p.exists():
            ok(f"Found  {hdr}")
        else:
            hard_missing.append(hdr)

    for src in list(PROJECT_SOURCES):
        p = PROJECT_ROOT / src
        if p.exists():
            ok(f"Found  {src}")
        else:
            soft_missing.append(src)

    for src in IMGUI_SOURCES:
        p = PROJECT_ROOT / src
        if p.exists():
            ok(f"Found  {src}")
        else:
            hard_missing.append(src)

    if soft_missing:
        warn("Source files not found — will be skipped:")
        for m in soft_missing:
            print(f"         {m}")
            PROJECT_SOURCES.remove(m)

    if hard_missing:
        err("Required files missing — cannot build:")
        for m in hard_missing:
            print(f"         {m}")
        return False

    return True


# ─────────────────────────────────────────────────────────────────
#  Compile flags
# ─────────────────────────────────────────────────────────────────

def build_flags(env: BuildEnv, debug: bool, as_dll: bool) -> List[str]:
    inc_flags = [f"/I{d}" for d in env.include_dirs]

    defines = [
        "/D", "WIN32",
        "/D", "_WINDOWS",
        "/D", "UNICODE",
        "/D", "_UNICODE",
        "/D", "WIN32_LEAN_AND_MEAN",
        "/D", "_CRT_SECURE_NO_WARNINGS",
        *(["/D", "ENABLE_CONSOLE"] if debug else []),
    ]

    optimise = ["/Od", "/Zi"] if debug else ["/O2", "/GL"]

    warn_flags = [
        "/W3",
        "/wd4996",   # deprecated CRT
        "/wd4244",   # float/double narrowing
        "/wd4267",   # size_t narrowing
        "/wd4005",   # macro redefinition (ImGui vs windows.h)
        "/wd4018",   # signed/unsigned comparison
        "/wd4100",   # unreferenced formal parameter
        "/wd4189",   # local variable initialized but not referenced
    ]

    misc = [
        "/nologo",
        "/EHsc",
        "/std:c++17",
        "/MT" if not debug else "/MTd",
        "/MP",       # parallel compilation
    ]

    return misc + optimise + warn_flags + inc_flags + defines


def link_flags(env: BuildEnv, debug: bool, as_dll: bool,
               out_path: Path, pdb_path: Path) -> List[str]:
    lib_paths = [f"/LIBPATH:{d}" for d in env.lib_dirs]

    system_libs = [
        "kernel32.lib", "user32.lib",   "gdi32.lib",
        "d3d11.lib",    "dxgi.lib",     "dwmapi.lib",
        "shell32.lib",  "ole32.lib",    "ntdll.lib",
        "advapi32.lib", "winmm.lib",
    ]

    flags = [
        "/nologo",
        f"/OUT:{out_path}",
        f"/PDB:{pdb_path}",
        "/MACHINE:X64",
        "/SUBSYSTEM:WINDOWS",
    ]

    if as_dll:
        flags += ["/DLL"]
    if debug:
        flags += ["/DEBUG:FULL"]
    else:
        flags += ["/LTCG", "/OPT:REF", "/OPT:ICF"]

    return flags + lib_paths + system_libs


# ─────────────────────────────────────────────────────────────────
#  Compilation + linking
# ─────────────────────────────────────────────────────────────────

def compile_sources(env: BuildEnv, sources: List[Path], obj_dir: Path,
                    cl_flags: List[str]) -> Tuple[bool, List[Path]]:
    obj_dir.mkdir(parents=True, exist_ok=True)
    obj_files: List[Path] = []
    failed = 0

    for src in sources:
        obj   = obj_dir / (src.stem + ".obj")
        extra = PER_SOURCE_FLAGS.get(src.name, [])
        cmd   = [str(env.cl), *cl_flags, *extra,
                 "/c", f"/Fo{obj}", str(src)]

        t0     = time.perf_counter()
        result = subprocess.run(cmd, capture_output=True, text=True,
                                env=env.env, cwd=PROJECT_ROOT)
        elapsed = time.perf_counter() - t0

        rel = src.relative_to(PROJECT_ROOT)
        if result.returncode != 0:
            err(f"FAILED  {rel}  ({elapsed:.2f}s)")
            for line in (result.stdout + result.stderr).splitlines():
                if line.strip():
                    print(f"    {line}")
            failed += 1
        else:
            ok(f"Compiled  {rel}  ({elapsed:.2f}s)")
            obj_files.append(obj)

    return failed == 0, obj_files


def kill_running(bin_path: Path) -> None:
    result = subprocess.run(
        ["taskkill", "/F", "/IM", bin_path.name],
        capture_output=True, text=True
    )
    if "SUCCESS" in result.stdout:
        ok(f"Killed running instance of {bin_path.name}")


def link_objects(env: BuildEnv, obj_files: List[Path],
                 lnk_flags: List[str]) -> bool:
    cmd    = [str(env.link)] + lnk_flags + [str(o) for o in obj_files]
    t0     = time.perf_counter()
    result = subprocess.run(cmd, capture_output=True, text=True,
                            env=env.env, cwd=PROJECT_ROOT)
    elapsed = time.perf_counter() - t0

    if result.returncode != 0:
        err(f"Link failed ({elapsed:.2f}s)")
        for line in result.stdout.splitlines():
            if line.strip():
                print(f"    {line}")
        return False
    ok(f"Linked  ({elapsed:.2f}s)")
    return True


# ─────────────────────────────────────────────────────────────────
#  Post-build
# ─────────────────────────────────────────────────────────────────

def embed_manifest(bin_path: Path, env: BuildEnv) -> None:
    manifest = PROJECT_ROOT / "app.manifest"
    if not manifest.exists():
        warn("app.manifest not found — UAC elevation won't auto-trigger")
        return

    mt_candidates = list(
        (env.sdk_root / "bin" / env.sdk_ver / "x64").glob("mt.exe")
    ) or list(env.sdk_root.rglob("mt.exe"))

    if not mt_candidates:
        warn("mt.exe not found — skipping manifest embed")
        return

    mt = mt_candidates[0]
    result = subprocess.run(
        [str(mt), "-nologo",
         "-manifest", str(manifest),
         f"-outputresource:{bin_path};#1"],
        capture_output=True, text=True
    )
    if result.returncode == 0:
        ok("Manifest embedded (UAC elevation enabled)")
    else:
        warn(f"Manifest embed failed: {result.stderr.strip()}")


def strip_pdb(pdb_path: Path) -> None:
    if pdb_path.exists():
        pdb_path.unlink()
        ok(f"Stripped PDB: {pdb_path.name}")


def compress_upx(bin_path: Path) -> None:
    if not shutil.which("upx"):
        warn("UPX not on PATH — skipping compression")
        return
    t0 = time.perf_counter()
    result = subprocess.run(
        ["upx", "--best", "--no-lzma", str(bin_path)],
        capture_output=True, text=True
    )
    if result.returncode == 0:
        ok(f"UPX compressed  ({time.perf_counter()-t0:.2f}s)")
    else:
        warn(f"UPX failed: {result.stderr.strip()}")


def print_summary(bin_path: Path, total: float) -> None:
    head("Build Summary")
    if bin_path.exists():
        ok(f"Output : {bin_path}")
        ok(f"Size   : {bin_path.stat().st_size / 1024:.1f} KB")
    ok(f"Time   : {total:.2f}s")

    head("File overview")
    groups = [
        ("Dispatchers (compiled)",  ["esp.cpp", "menu.cpp"]),
        ("ESP themes (header-only)", ["esp_theme_normal.h"]),
        ("Menu themes (header-only)",["menu_theme_industrial.h",
                                      "menu_theme_original.h"]),
        ("Aimbot",                   ["aimbot.cpp", "aimbot_settings_patch.h"]),
        ("Shared state",             ["x9_shared.h"]),
    ]
    for label, files in groups:
        info(label)
        for f in files:
            p = PROJECT_ROOT / f
            if p.exists():
                print(f"       {f:44s}  {p.stat().st_size/1024:6.1f} KB")
            else:
                print(f"       {f:44s}  [missing]")


# ─────────────────────────────────────────────────────────────────
#  Entry point
# ─────────────────────────────────────────────────────────────────

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="x9 build script",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("--exe",         action="store_true", default=True,
                   help="Build as standalone EXE (default)")
    p.add_argument("--dll",         action="store_true",
                   help="Build as injectable DLL")
    p.add_argument("--debug",       action="store_true",
                   help="Debug: no optimisation, PDB kept, console enabled")
    p.add_argument("--clean",       action="store_true",
                   help="Delete build/ and exit")
    p.add_argument("--no-compress", action="store_true",
                   help="Skip UPX compression")
    p.add_argument("--no-sync",     action="store_true",
                   help="Skip syncing outputs → project sources")
    p.add_argument("--outdir",      default=str(BUILD_DIR),
                   help=f"Output directory (default: {BUILD_DIR})")
    p.add_argument("--name",        default=DEFAULT_OUT_NAME,
                   help=f"Binary name without extension (default: {DEFAULT_OUT_NAME})")
    p.add_argument("--outputs-dir", default=None,
                   help="Directory containing updated outputs to sync "
                        "(default: <script_dir>/outputs if it exists)")
    return p.parse_args()


def main() -> int:
    args    = parse_args()
    out_dir = Path(args.outdir)
    t_start = time.perf_counter()

    head("x9  —  Build System  (theme-aware)")

    # ── Clean ──────────────────────────────────────────────────────
    if args.clean:
        if BUILD_DIR.exists():
            shutil.rmtree(BUILD_DIR)
            ok(f"Deleted {BUILD_DIR}")
        else:
            info("build/ does not exist — nothing to clean")
        return 0

    # ── Sync outputs ───────────────────────────────────────────────
    if not args.no_sync:
        outputs_dir = (
            Path(args.outputs_dir) if args.outputs_dir
            else SCRIPT_DIR / "outputs"
        )
        sync_outputs(outputs_dir)

    # ── Validate sources ──────────────────────────────────────────
    if not validate_sources():
        return 1

    # ── Locate toolchain ──────────────────────────────────────────
    head("Locating Build Tools")
    env = BuildEnv()
    if not env.locate():
        return 1

    # ── Prepare dirs / paths ──────────────────────────────────────
    obj_dir  = out_dir / "obj"
    obj_dir.mkdir(parents=True, exist_ok=True)

    as_dll   = args.dll
    ext      = ".dll" if as_dll else ".exe"
    bin_path = out_dir / (args.name + ext)
    pdb_path = out_dir / (args.name + ".pdb")

    # ── Build flags ───────────────────────────────────────────────
    cl_flags  = build_flags(env, args.debug, as_dll)
    lnk_flags = link_flags(env, args.debug, as_dll, bin_path, pdb_path)

    # ── Compile ───────────────────────────────────────────────────
    head("Compiling Sources")
    all_sources = (
        [PROJECT_ROOT / s for s in PROJECT_SOURCES]
        + [PROJECT_ROOT / s for s in IMGUI_SOURCES]
    )

    success, obj_files = compile_sources(env, all_sources, obj_dir, cl_flags)
    if not success or not obj_files:
        err("Compilation failed — aborting")
        return 1

    # ── Link ──────────────────────────────────────────────────────
    head("Linking")
    kill_running(bin_path)
    if not link_objects(env, obj_files, lnk_flags):
        return 1

    # ── Post-build ────────────────────────────────────────────────
    head("Post-Build")
    embed_manifest(bin_path, env)
    if not args.debug:
        strip_pdb(pdb_path)
        if not args.no_compress:
            compress_upx(bin_path)
    else:
        ok(f"PDB kept for debugging: {pdb_path.name}")

    # ── Summary ───────────────────────────────────────────────────
    print_summary(bin_path, time.perf_counter() - t_start)
    return 0


if __name__ == "__main__":
    sys.exit(main())
