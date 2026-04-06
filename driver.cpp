/*
 * driver.cpp - Memory access with NullKDriver + usermode fallback
 *
 * Init() tries to connect to the kernel driver via NtQueryCompositionSurfaceStatistics.
 * If the driver is not loaded, it falls back to ReadProcessMemory / WriteProcessMemory.
 * SeDebugPrivilege is acquired on startup so OpenProcess succeeds on protected processes.
 */

#include "driver.h"
#include "console.h"
#include "roblox_offsets.h"
#include <TlHelp32.h>
#include <winternl.h>
#include <algorithm>

#pragma comment(lib, "ntdll.lib")

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS      ((NTSTATUS)0x00000000L)
#endif
#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

typedef NTSTATUS(NTAPI* fnNtQueryCompositionSurfaceStatistics)(PVOID);

static fnNtQueryCompositionSurfaceStatistics g_CallDriver = nullptr;
static bool   g_UseDriver  = false;
static bool   g_Connected  = false;
static HANDLE g_hProcess   = NULL;
static DWORD  g_CachedPid  = 0;

/* ── Acquire SeDebugPrivilege ─────────────────────────────────────
 * Without this, OpenProcess on a protected process returns NULL
 * even when running as administrator.                              */
static void AcquireDebugPrivilege()
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return;

    LUID luid{};
    if (LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &luid)) {
        TOKEN_PRIVILEGES tp{};
        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    }
    CloseHandle(hToken);
}

static NTSTATUS SendRequest(REQUEST_DATA* req) {
    if (!g_CallDriver) return STATUS_UNSUCCESSFUL;
    return g_CallDriver((PVOID)req);
}

static bool EnsureHandle(DWORD pid) {
    if (g_CachedPid == pid && g_hProcess) return true;
    if (g_hProcess) { CloseHandle(g_hProcess); g_hProcess = NULL; }

    /* Try full access first */
    g_hProcess = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE, pid);

    /* Fallback: read-only handle if write was denied */
    if (!g_hProcess) {
        g_hProcess = OpenProcess(
            PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
            FALSE, pid);
        if (g_hProcess)
            Console::Warn("Driver: opened read-only handle (writes will fail)");
    }

    if (!g_hProcess)
        Console::Error("Driver: OpenProcess(%lu) failed — GLE=%lu", pid, GetLastError());

    g_CachedPid = pid;
    return g_hProcess != NULL;
}

bool Driver::Init() {
    /* Must be first — enables OpenProcess on protected targets */
    AcquireDebugPrivilege();

    /* Try kernel driver */
    HMODULE hWin32u = GetModuleHandleW(L"win32u.dll");
    if (!hWin32u) hWin32u = LoadLibraryW(L"win32u.dll");
    if (hWin32u) {
        g_CallDriver = (fnNtQueryCompositionSurfaceStatistics)
            GetProcAddress(hWin32u, "NtQueryCompositionSurfaceStatistics");
    }

    if (g_CallDriver) {
        REQUEST_DATA req{};
        req.magic = REQUEST_MAGIC;
        req.command = CMD_PING;
        if (SendRequest(&req) == STATUS_SUCCESS) {
            if (req.result == 0x50544548 || req.result == 0x4B524E4C) {
                Console::Success("Kernel driver connected (0x%llX)", req.result);
                g_UseDriver = true;
                g_Connected = true;
                return true;
            }
        }
        Console::Warn("Driver ping failed — falling back to usermode (Safe Mode compatible)");
    } else {
        Console::Warn("Driver hook not found — using usermode ReadProcessMemory");
    }

    /* Usermode fallback — works in Safe Mode */
    g_UseDriver = false;
    g_Connected = true;
    Console::Success("Usermode memory access ready");
    return true;
}

bool Driver::IsConnected() { return g_Connected; }

uintptr_t Driver::GetModuleBase(DWORD pid, const wchar_t* moduleName) {
    if (g_UseDriver && g_CallDriver) {
        REQUEST_DATA req{};
        req.magic = REQUEST_MAGIC;
        req.command = CMD_MODULE_BASE;
        req.pid = (unsigned __int64)pid;
        wcsncpy_s(req.module_name, 64, moduleName, _TRUNCATE);
        if (SendRequest(&req) == STATUS_SUCCESS && req.result)
            return (uintptr_t)req.result;
    }

    /* Usermode toolhelp snapshot fallback */
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W me{}; me.dwSize = sizeof(me);
    uintptr_t base = 0;
    if (Module32FirstW(hSnap, &me)) {
        do {
            if (_wcsicmp(me.szModule, moduleName) == 0) {
                base = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32NextW(hSnap, &me));
    }
    CloseHandle(hSnap);
    return base;
}

bool Driver::ReadRaw(DWORD pid, uintptr_t address, void* buffer, size_t size) {
    if (!g_Connected || !buffer || !size || !address) return false;

    if (g_UseDriver && g_CallDriver) {
        REQUEST_DATA req{};\
        req.magic = REQUEST_MAGIC; req.command = CMD_READ;
        req.pid = (unsigned __int64)pid;
        req.address = (unsigned __int64)address;
        req.buffer = (unsigned __int64)buffer;
        req.size = (unsigned __int64)size;
        if (SendRequest(&req) == STATUS_SUCCESS && req.result) return true;
    }

    if (!EnsureHandle(pid)) return false;
    SIZE_T bytesRead = 0;
    bool ok = ReadProcessMemory(g_hProcess, (LPCVOID)address, buffer, size, &bytesRead)
              && bytesRead == size;

    /* One-time diagnostic on first failure */
    if (!ok) {
        static bool s_logged = false;
        if (!s_logged) {
            s_logged = true;
            DWORD gle = GetLastError();
            Console::Warn("[Driver] ReadProcessMemory FAILED addr=0x%llX size=%llu GLE=%lu handle=%p",
                (unsigned long long)address, (unsigned long long)size, gle, g_hProcess);
            /* GLE=5  = Access Denied (need SeDebugPrivilege or higher integrity)
             * GLE=299= Partial read (bad address range)
             * GLE=87 = Invalid parameter                                       */
        }
    }
    return ok;
}

bool Driver::WriteRaw(DWORD pid, uintptr_t address, void* buffer, size_t size) {
    if (!g_Connected || !buffer || !size || !address) return false;

    if (g_UseDriver && g_CallDriver) {
        REQUEST_DATA req{};
        req.magic = REQUEST_MAGIC; req.command = CMD_WRITE;
        req.pid = (unsigned __int64)pid;
        req.address = (unsigned __int64)address;
        req.buffer = (unsigned __int64)buffer;
        req.size = (unsigned __int64)size;
        if (SendRequest(&req) == STATUS_SUCCESS && req.result) return true;
    }

    if (!EnsureHandle(pid)) return false;
    SIZE_T bytesWritten = 0;
    return WriteProcessMemory(g_hProcess, (LPVOID)address, buffer, size, &bytesWritten)
           && bytesWritten == size;
}

std::string Driver::ReadRobloxString(DWORD pid, uintptr_t instanceAddr, uintptr_t nameOffset) {
    uintptr_t nameAddr = instanceAddr + nameOffset;
    uint32_t len = Read<uint32_t>(pid, nameAddr + 0x10);
    if (len == 0 || len > 256) return "";

    char buf[257] = {};
    if (len <= 15) {
        ReadRaw(pid, nameAddr, buf, len);
    } else {
        uintptr_t strPtr = Read<uintptr_t>(pid, nameAddr);
        if (!strPtr) return "";
        ReadRaw(pid, strPtr, buf, (std::min)(len, (uint32_t)256));
    }
    buf[(std::min)(len, (uint32_t)256)] = '\0';
    return std::string(buf);
}