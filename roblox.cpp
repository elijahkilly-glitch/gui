/*
 * roblox.cpp - Roblox data model traversal
 *
 * Traverses: DataModel -> Players -> Characters -> HumanoidRootPart/Head
 * Also reads the Camera ViewMatrix for world-to-screen projection.
 */

#include "roblox.h"
#include "driver.h"
#include "console.h"
#include "esp.h"
#include "roblox_offsets.h"
#include <TlHelp32.h>
#include <cmath>
#include <map>
#include <cctype> /* isdigit */
#include <thread>
#include <atomic>
#include <mutex>

/* DisplayName: offsets::Player::DisplayName = 0x130 (version-ae421f0582e54718) */

static std::atomic<bool> g_scannerRunning = false;
static std::thread g_scannerThread;
static std::mutex g_playerListMutex; // Protects g_playerCache

static DWORD       g_pid = 0;
static uintptr_t   g_base = 0;
static HWND        g_hwnd = NULL;
static float        g_viewMatrix[16] = {};
static std::vector<PlayerData> g_players;
static int          g_screenW = 0;
static int          g_screenH = 0;

/* ── Globals for Game State & Caching ──────────────────────────── */
static uintptr_t    g_lastLocalPlayer = 0;
static uintptr_t    g_localHumanoid = 0;
static uintptr_t    g_localRootPart = 0;  /* HumanoidRootPart primitive ptr */
static uintptr_t    g_lastCharacter = 0;
static uintptr_t    g_currentDataModel = 0;
static uintptr_t    g_playersService = 0;
static uintptr_t    g_workspaceService = 0;
static uintptr_t    g_lightingService  = 0;
static DWORD        g_lastPlayerUpdate = 0;
static DWORD        g_lastEntityUpdate = 0;
static std::map<long long, std::string> g_idToName;

/* ── Helper: Find Roblox PID ───────────────────────────────────── */

static DWORD FindRobloxPID() {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"RobloxPlayerBeta.exe") == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return pid;
}

/* ── Helper: Find Roblox Window ────────────────────────────────── */

struct EnumData { DWORD pid; HWND result; };

static BOOL CALLBACK EnumWindowProc(HWND hwnd, LPARAM lp) {
    EnumData* data = (EnumData*)lp;
    DWORD wndPid = 0;
    GetWindowThreadProcessId(hwnd, &wndPid);

    if (wndPid == data->pid && IsWindowVisible(hwnd)) {
        char title[256] = {};
        GetWindowTextA(hwnd, title, 256);
        if (strlen(title) > 0) {
            data->result = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

static HWND FindRobloxWindow(DWORD pid) {
    EnumData data{ pid, NULL };
    EnumWindows(EnumWindowProc, (LPARAM)&data);
    return data.result;
}

/* ── Helper: Get class name ──────────────────────────────────────── */

static std::string GetRobloxClassName(DWORD pid, uintptr_t instance) {
    if (!instance) return "";

    uintptr_t classDesc = Driver::Read<uintptr_t>(pid, instance + offsets::Instance::ClassDescriptor);
    if (!classDesc) return "";

    /*
       ClassDescriptor + 0x8 contains a POINTER to the string struct.
       We must dereference it first.
    */
    uintptr_t stringPtr = Driver::Read<uintptr_t>(pid, classDesc + offsets::Instance::ClassName);
    if (!stringPtr) return "";

    return Driver::ReadRobloxString(pid, stringPtr, 0);
}

/* ── Helper: Get position from a Part (CFrame) ────────────────── */

static bool GetPartPosition(DWORD pid, uintptr_t part, float outPos[3]) {
    if (!part) return false;

    uintptr_t primitive = Driver::Read<uintptr_t>(pid, part + offsets::BasePart::Primitive);
    if (!primitive) return false;

    /* CFrame at Primitive + CFrame offset. Position is at CFrame + 0x0C*3 = the translation part */
    /* CFrame layout: 3x3 rotation matrix (9 floats) then 3 position floats */
    /* So position is at CFrame + 36 bytes (9 * 4) = offset 0x24 from CFrame start */
    /* Actually in Roblox, the CFrame stores rotation 3x3 then position xyz */

    float cframe[12] = {};
    Driver::ReadRaw(pid, primitive + offsets::Prim::CFrame, cframe, sizeof(cframe));

    /* CFrame memory layout: [r00, r01, r02, r10, r11, r12, r20, r21, r22, px, py, pz] */
    outPos[0] = cframe[9];   /* X */
    outPos[1] = cframe[10];  /* Y */
    outPos[2] = cframe[11];  /* Z */

    return (outPos[0] != 0.0f || outPos[1] != 0.0f || outPos[2] != 0.0f);
}

/* ── Helper: Get children list ─────────────────────────────────── */

static std::vector<uintptr_t> GetChildren(DWORD pid, uintptr_t parent) {
    std::vector<uintptr_t> result;
    if (!parent) return result;

    /*
       Roblox Children Vector Structure:
       instance + 0x70 -> Pointer to [begin, end, cap]
       Each entry is 16 bytes (shared_ptr): [instance_ptr, control_block_ptr]
    */
    uintptr_t vectorStruct = Driver::Read<uintptr_t>(pid, parent + offsets::Instance::ChildrenStart);
    if (!vectorStruct) return result;

    uintptr_t begin = Driver::Read<uintptr_t>(pid, vectorStruct + 0x0);
    uintptr_t end = Driver::Read<uintptr_t>(pid, vectorStruct + 0x8);

    if (!begin || !end || end <= begin) return result;

    size_t bytes = end - begin;
    size_t count = bytes / 16;

    if (count > 10000) count = 10000; // Cap at 10k to prevent crazy memory usage
    if (count == 0) return result;

    /* Bulk Read Optimization */
    std::vector<unsigned char> buffer(count * 16);
    if (Driver::ReadRaw(pid, begin, buffer.data(), buffer.size())) {
        for (size_t i = 0; i < count; i++) {
            // Child pointer is at offset 0 of each 16-byte chunk
            uintptr_t child = *reinterpret_cast<uintptr_t*>(&buffer[i * 16]);
            if (child) result.push_back(child);
        }
    }

    return result;
}

static uintptr_t FindChildByClass(DWORD pid, uintptr_t parent, const std::string& className) {
    auto children = GetChildren(pid, parent);
    // Debug log only for rare events or first run
    static int logCount = 0;
    bool debug = (logCount < 50 && (className == "Players" || className == "Workspace"));

    if (debug) Console::Log("FindChildByClass: Searching for '%s' in %llX (Children: %d)", className.c_str(), parent, (int)children.size());

    for (auto& child : children) {
        std::string cls = GetRobloxClassName(pid, child);
        if (debug && logCount < 50) {
            Console::Log("  > Arg: %llX | Class: '%s' vs '%s'", child, cls.c_str(), className.c_str());
        }
        if (cls == className) {
            if (debug) {
                Console::Success("  > MATCH! Found %llX", child);
                logCount++;
            }
            return child;
        }
    }
    if (debug) logCount++;
    return 0;
}

static uintptr_t FindChildByName(DWORD pid, uintptr_t parent, const std::string& name) {
    auto children = GetChildren(pid, parent);
    for (auto& child : children) {
        // Indirect Name Scan: Ptr -> String
        uintptr_t namePtr = Driver::Read<uintptr_t>(pid, child + offsets::Instance::Name);
        if (namePtr) {
            std::string s = Driver::ReadRobloxString(pid, namePtr, 0);
            if (s == name) return child;
        }
    }
    return 0;
}

/* ── Public API ──────────────────────────────────────────────────── */

/* ── Performance Caching Structs ─────────────────────────────── */

struct CachedPart {
    uintptr_t ptr;
    uintptr_t primitive; /* Optimization: Cache Primitive to skip 1 read level */
};

struct CachedPlayer {
    uintptr_t ptr;
    std::string name;
    std::string displayName;   /* Roblox DisplayName property */
    uintptr_t character; /* Current ModelInstance */
    uintptr_t humanoid;  /* Cached Humanoid Instance */

    /* Cached Parts (Valid if character != 0) */
    CachedPart rootPart;
    CachedPart head;
    CachedPart lFoot, rFoot, lHand, rHand;

    bool hasLimbs;
    bool valid;
    bool isEntity;       /* True if found via Workspace scan (not in Players service) */
    bool headIsFallback; /* True if head was not found — retry FindChildByName each frame */
};

static std::vector<CachedPlayer> g_playerCache;
static DWORD g_lastCacheUpdate = 0;
static DWORD g_lastDebugUpdate = 0;
static bool  g_debugEnabled = true; // Enable debug by default for troubleshooting

/* ── Helper: Get Position from Cached Part ───────────────────── */

static bool GetPartPositionCached(DWORD pid, const CachedPart& part, float outPos[3]) {
    if (!part.primitive) return false;

    float cframe[12] = {};
    if (!Driver::ReadRaw(pid, part.primitive + offsets::Prim::CFrame, cframe, sizeof(cframe))) return false;

    outPos[0] = cframe[9];
    outPos[1] = cframe[10];
    outPos[2] = cframe[11];

    return (outPos[0] != 0.0f || outPos[1] != 0.0f || outPos[2] != 0.0f);
}

/* ── Helper: Cache a Part ────────────────────────────────────── */

static CachedPart CachePart(DWORD pid, uintptr_t partPtr) {
    CachedPart cp = { 0, 0 };
    if (partPtr) {
        cp.ptr = partPtr;
        cp.primitive = Driver::Read<uintptr_t>(pid, partPtr + offsets::BasePart::Primitive);
    }
    return cp;
}

/* ── Public API ──────────────────────────────────────────────── */

bool Roblox::Init() {
    g_pid = FindRobloxPID();
    if (!g_pid) {
        Console::Error("Roblox process not found");
        return false;
    }
    Console::Success("Found Roblox PID: %d", g_pid);

    g_base = Driver::GetModuleBase(g_pid, L"RobloxPlayerBeta.exe");
    if (!g_base) {
        Console::Error("Failed to get Roblox module base");
        return false;
    }
    Console::Success("Roblox base: 0x%llX", g_base);

    g_hwnd = FindRobloxWindow(g_pid);
    if (!g_hwnd) {
        Console::Warn("Roblox window not found yet");
    }
    else {
        Console::Success("Roblox window found: 0x%p", g_hwnd);
    }

    return true;
}

/* ── Update Logic ────────────────────────────────────────────── */

static std::string g_localPlayerName = "";

void Roblox::Update() {
    if (!g_pid || !g_base) return;

    DWORD now = GetTickCount();
    uintptr_t localPlayer = 0;

    /* Re-find window if needed */
    if (!g_hwnd || !IsWindow(g_hwnd)) {
        g_hwnd = FindRobloxWindow(g_pid);
    }
    if (g_hwnd) {
        RECT r;
        GetClientRect(g_hwnd, &r);
        g_screenW = r.right - r.left;
        g_screenH = r.bottom - r.top;
    }

    /* ── Standard Update Logic ──────────────────────────────────── */
    uintptr_t visualEngine = Driver::Read<uintptr_t>(g_pid, g_base + offsets::VEngine::Pointer);
    if (visualEngine) {
        Driver::ReadRaw(g_pid, visualEngine + offsets::VEngine::ViewMatrix, g_viewMatrix, sizeof(g_viewMatrix));
    }

    uintptr_t fakeDataModel = Driver::Read<uintptr_t>(g_pid, g_base + offsets::FakeDataModel::Pointer);
    if (fakeDataModel) {
        uintptr_t dataModel = Driver::Read<uintptr_t>(g_pid, fakeDataModel + offsets::FakeDataModel::RealDataModel);
        if (dataModel) {
            if (dataModel != g_currentDataModel) {
                /* DataModel Changed (Server Switch) - Reset Everything */
                Console::Warn("[Roblox] DataModel mismatch (0x%llX -> 0x%llX). Resetting cache.", g_currentDataModel, dataModel);
                g_currentDataModel = dataModel;

                std::lock_guard<std::mutex> lock(g_playerListMutex);
                g_playersService = 0;
                g_workspaceService = 0;
                g_lightingService  = 0;
                g_localHumanoid = 0;
                g_localRootPart = 0;
                g_lastLocalPlayer = 0;
                g_lastCharacter = 0;
                g_playerCache.clear();
                g_idToName.clear();
                g_localPlayerName = "";
            }

            /* ── Find LocalPlayer ── */
            if (!g_playersService)  g_playersService  = FindChildByClass(g_pid, dataModel, "Players");
            if (!g_lightingService)  g_lightingService  = FindChildByClass(g_pid, dataModel, "Lighting");
            if (g_playersService) {
                localPlayer = Driver::Read<uintptr_t>(g_pid, g_playersService + offsets::Players::LocalPlayer);
                if (localPlayer) {
                    if (localPlayer != g_lastLocalPlayer) {
                        g_lastLocalPlayer = localPlayer;
                        g_localHumanoid = 0;
                        g_lastCharacter = 0;
                        g_localPlayerName = "";

                        uintptr_t namePtr = Driver::Read<uintptr_t>(g_pid, localPlayer + offsets::Instance::Name);
                        if (namePtr) g_localPlayerName = Driver::ReadRobloxString(g_pid, namePtr, 0);
                    }

                    uintptr_t character = Driver::Read<uintptr_t>(g_pid, localPlayer + offsets::Player::Character);
                    if (character != g_lastCharacter) {
                        g_lastCharacter = character;
                        g_localHumanoid = 0;
                        g_localRootPart = 0;
                    }

                    if (!g_localHumanoid && character) {
                        g_localHumanoid = FindChildByClass(g_pid, character, "Humanoid");
                    }
                    if (!g_localRootPart && character) {
                        uintptr_t rootInst = FindChildByName(g_pid, character, "HumanoidRootPart");
                        if (rootInst) g_localRootPart = Driver::Read<uintptr_t>(g_pid, rootInst + offsets::BasePart::Primitive);
                    }
                }
            }
        }
    }

    /* ── Apply Movement & Player Hacks ──────────────────────────────── */
    if (g_localHumanoid) {
        static DWORD lastAfkMove = 0;
        static DWORD lastRegenTime = 0;
        static float lastHealth = -1.0f;
        static DWORD lastDmgTime = 0;
        DWORD nowHack = GetTickCount();

        /* WalkSpeed */
        if (g_cfg.movement.useSpeed || g_cfg.movement.alwaysSprint) {
            float spd = g_cfg.movement.alwaysSprint ? 22.0f : g_cfg.movement.walkSpeed;
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkSpeed, spd);
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkSpeedCheck, spd);
        }

        /* JumpPower / InfiniteJump */
        if (g_cfg.movement.useJump || g_cfg.movement.infiniteJump) {
            float jp = g_cfg.movement.infiniteJump ? 200.0f : g_cfg.movement.jumpPower;
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::JumpPower, jp);
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::JumpHeight, jp);
        }

        /* BunnyHop — sets high speed + jump each tick so player hops continuously */
        if (g_cfg.movement.bunnyHop) {
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::JumpPower, 60.0f);
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkSpeed, 28.0f);
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkSpeedCheck, 28.0f);
        }

        /* No slip — max slope angle so player can walk up walls */
        if (g_cfg.movement.noSlip) {
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::MaxSlopeAngle, 89.9f);
        }

        /* Low gravity — scale hip height and walk speed to simulate lower gravity */
        if (g_cfg.movement.lowGravity) {
            float scaledJump = 100.0f / (g_cfg.movement.gravityScale + 0.01f);
            float scaledJumpCap = (scaledJump > 500.0f) ? 500.0f : scaledJump;
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::JumpPower, scaledJumpCap);
            Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::JumpHeight, scaledJumpCap * 0.5f);
        }

        /* God mode — constantly write max health to current health */
        if (g_cfg.movement.godMode) {
            float mhp = Driver::Read<float>(g_pid, g_localHumanoid + offsets::Humanoid::MaxHealth);
            if (mhp > 0.0f) Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::Health, mhp);
        }

        /* HP Regen — detect damage, wait delay, then restore HP per second */
        if (g_cfg.movement.regenHP && !g_cfg.movement.godMode) {
            float curHp = Driver::Read<float>(g_pid, g_localHumanoid + offsets::Humanoid::Health);
            float maxHp = Driver::Read<float>(g_pid, g_localHumanoid + offsets::Humanoid::MaxHealth);
            if (lastHealth > 0.0f && curHp < lastHealth) lastDmgTime = nowHack;
            if (maxHp > 0.0f && curHp < maxHp &&
                (nowHack - lastDmgTime) >(DWORD)(2000)) {  /* 2s delay */
                static DWORD lastRegenTick = 0;
                if (nowHack - lastRegenTick > 100) {  /* tick every 100ms */
                    float add = (g_cfg.movement.regenRate * 0.1f);
                    float newHp = (curHp + add > maxHp) ? maxHp : curHp + add;
                    Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::Health, newHp);
                    lastRegenTick = nowHack;
                }
            }
            lastHealth = curHp;
        }

        /* Anti-AFK — tiny walk direction toggle every 4 minutes */
        if (g_cfg.movement.antiAFK) {
            static DWORD lastAfkTick = 0;
            static bool  afkToggle = false;
            if (nowHack - lastAfkTick > 240000) {  /* 4 min */
                /* Write a tiny offset to WalkToPoint to simulate input */
                float wx = afkToggle ? 0.01f : -0.01f;
                Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkToPoint, wx);
                Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkToPoint + 4, 0.0f);
                Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkToPoint + 8, wx);
                afkToggle = !afkToggle;
                lastAfkTick = nowHack;
            }
        }

        /* Fly / Noclip — zero out velocity via primitive AssemblyLinearVelocity */
        if ((g_cfg.movement.noclip || g_cfg.movement.fly) && g_localRootPart) {
            /* Zero velocity to prevent physics from resisting */
            float zero3[3] = { 0,0,0 };
            Driver::WriteRaw(g_pid, g_localRootPart + offsets::Prim::AssemblyLinearVelocity, zero3, 12);
        }

        /* Anti-void — if player falls below Y threshold, write CFrame Y back up */
        if (g_cfg.movement.antiVoid && g_localRootPart) {
            float cframe[12];
            if (Driver::ReadRaw(g_pid, g_localRootPart + offsets::Prim::Position, cframe, 12)) {
                float y = cframe[1];
                if (y < g_cfg.movement.antiVoidY) {
                    /* Write Y position component */
                    float safeY = 100.0f;
                    Driver::Write<float>(g_pid, g_localRootPart + offsets::Prim::Position + 4, safeY);
                }
            }
        }
    }

    /* ── Update Existing Players (High Frequency) ────────────── */
    {
        std::lock_guard<std::mutex> lock(g_playerListMutex); // Thread-Safe Access
        g_players.clear();

        for (auto& cp : g_playerCache) {
            if (!cp.ptr) continue;

            PlayerData pd = {};
            pd.ptr = cp.ptr;
            pd.name = cp.name;
            pd.displayName = cp.displayName.empty() ? cp.name : cp.displayName;
            pd.humanoid = cp.humanoid;

            /* Re-read DisplayName for real players if it's still empty
               (covers players who joined before the scanner ran the first read) */
            if (!cp.isEntity && cp.displayName.empty()) {
                uintptr_t dispPtr = Driver::Read<uintptr_t>(g_pid, cp.ptr + offsets::Player::DisplayName);
                if (dispPtr) {
                    cp.displayName = Driver::ReadRobloxString(g_pid, dispPtr, 0);
                    if (!cp.displayName.empty()) pd.displayName = cp.displayName;
                }
            }
            /* Expose userId so the menu player list can show it */
            pd.userId = cp.isEntity ? 0LL :
                Driver::Read<long long>(g_pid, cp.ptr + offsets::Player::UserId);

            /* PF Robust Self Check */
            pd.isLocalPlayer = (cp.ptr == localPlayer) || (cp.ptr == g_lastCharacter) || (cp.character == g_lastCharacter);

            // As a last resort for custom rig games, if character names match EXACTLY, it's likely us
            if (!pd.isLocalPlayer && !cp.name.empty() && !g_localPlayerName.empty()) {
                if (cp.name == g_localPlayerName) pd.isLocalPlayer = true;
            }

            /*
               Keep LocalPlayer in the list so ESP can find its position for distance calculation.
               ESP loop will skip drawing based on isLocalPlayer flag.
            */
            // if (pd.isLocalPlayer) continue;

            pd.valid = false;

            /* Check Character */
            uintptr_t currentCharacter = 0;

            if (cp.isEntity) {
                // For entities, pointer IS the character
                currentCharacter = cp.ptr;
            }
            else {
                // For players, read Character property
                currentCharacter = Driver::Read<uintptr_t>(g_pid, cp.ptr + offsets::Player::Character);
            }

            /* FIX: re-run part discovery every frame while cp.valid is false.
               Previously only ran on character pointer change, so if parts weren't
               loaded on the first respawn frame the cache got stuck invalid forever.
               Also do a full reset (including humanoid) on character change so no
               stale health data from the old character leaks through. */
            if (currentCharacter != cp.character) {
                // Character changed (respawn/death) - fully reset
                cp.character = currentCharacter;
                cp.valid = false;
                cp.hasLimbs = false;
                cp.headIsFallback = false;
                cp.humanoid = 0;
                cp.rootPart = {};
                cp.head = {};
                cp.lFoot = {}; cp.rFoot = {};
                cp.lHand = {}; cp.rHand = {};
            }

            if (currentCharacter && !cp.valid) { // retry every frame until parts are found
                /* PF Root Discovery: fall through standard -> custom rig parts */
                uintptr_t root = FindChildByName(g_pid, currentCharacter, "HumanoidRootPart");
                if (!root) root = FindChildByName(g_pid, currentCharacter, "humanoid_root_part"); // Deadline custom rig
                if (!root) root = FindChildByName(g_pid, currentCharacter, "Torso");
                if (!root) root = FindChildByName(g_pid, currentCharacter, "Head");
                if (!root) root = FindChildByName(g_pid, currentCharacter, "head"); // Deadline

                if (!root) root = FindChildByName(g_pid, currentCharacter, "LowerTorso");
                if (!root) root = FindChildByName(g_pid, currentCharacter, "UpperTorso");
                if (!root) root = FindChildByName(g_pid, currentCharacter, "Body");
                if (!root) root = FindChildByName(g_pid, currentCharacter, "Head");

                uintptr_t head = FindChildByName(g_pid, currentCharacter, "Head");

                /* Tier 3 fallback: class-based discovery for obfuscated PF rigs */
                if (!root) {
                    auto charChildren = GetChildren(g_pid, currentCharacter);
                    uintptr_t firstPart = 0;
                    uintptr_t partWithChildren = 0;  // HRP typically has Motor6D joints as children
                    uintptr_t partNoChildren = 0;    // Head typically has 0 children

                    for (auto cc : charChildren) {
                        std::string ccCls = GetRobloxClassName(g_pid, cc);
                        if (ccCls == "Part" || ccCls == "MeshPart") {
                            if (!firstPart) firstPart = cc;
                            auto partCh = GetChildren(g_pid, cc);
                            if (partCh.size() >= 3 && !partWithChildren) {
                                partWithChildren = cc;  // Likely HumanoidRootPart or Torso (has joints)
                            }
                            if (partCh.size() == 0 && !partNoChildren) {
                                partNoChildren = cc;    // Likely Head (no children)
                            }
                        }
                    }

                    root = partWithChildren ? partWithChildren : firstPart;
                    if (!head) head = partNoChildren ? partNoChildren : firstPart;
                }

                /* Only fully re-cache if root is missing or changed, or first run */
                if (!cp.valid || cp.rootPart.ptr != root) {
                    cp.rootPart = CachePart(g_pid, root);
                    cp.head = CachePart(g_pid, head ? head : root);

                    /* If head wasn't found (mapped to root), flag so we keep
                       retrying FindChildByName each frame until it resolves   */
                    if (!head) cp.headIsFallback = true;
                    else cp.headIsFallback = false;

                    /* Scan Limbs (will fail on obfuscated rigs - that's OK, just no skeleton) */
                    auto GetLimb = [&](const char* r15, const char* r6) {
                        uintptr_t p = FindChildByName(g_pid, currentCharacter, r15);
                        if (!p) p = FindChildByName(g_pid, currentCharacter, r6);
                        return CachePart(g_pid, p);
                        };

                    cp.lFoot = GetLimb("LeftFoot", "Left Leg");
                    cp.rFoot = GetLimb("RightFoot", "Right Leg");
                    cp.lHand = GetLimb("LeftHand", "Left Arm");
                    cp.rHand = GetLimb("RightHand", "Right Arm");

                    cp.hasLimbs = (cp.lFoot.ptr && cp.rFoot.ptr);

                    cp.humanoid = FindChildByClass(g_pid, currentCharacter, "Humanoid");
                    cp.valid = (root != 0);
                }
            }

            if (!cp.character || !cp.valid) continue;

            /* If head was mapped to root previously, keep retrying every frame.
               This handles the case where Head loads a few frames after the
               character itself becomes valid (common on slower connections).   */
            if (cp.headIsFallback && cp.character) {
                uintptr_t freshHead = FindChildByName(g_pid, cp.character, "Head");
                if (freshHead) {
                    cp.head = CachePart(g_pid, freshHead);
                    cp.headIsFallback = false;
                }
            }

            /* Read Positions directly from Cached Primitives */
            if (GetPartPositionCached(g_pid, cp.rootPart, pd.rootPos)) {
                pd.valid = true;

                if (!GetPartPositionCached(g_pid, cp.head, pd.headPos)) {
                    /* Fallback A: head primitive invalid — estimate from HRP.
                       R15: head centre is ~3.5 studs above HumanoidRootPart. */
                    memcpy(pd.headPos, pd.rootPos, sizeof(pd.rootPos));
                    pd.headPos[1] += 3.5f;
                }
                /* Fallback B: head was mapped to the root primitive (head Part
                   wasn't found in the character, so CachePart used root).
                   GetPartPositionCached succeeds but headPos == rootPos exactly.
                   Detect this and apply the same upward offset so "Head" bone
                   selection actually aims above the torso.                    */
                else if (cp.head.primitive == cp.rootPart.primitive ||
                         (fabsf(pd.headPos[0]-pd.rootPos[0]) < 0.001f &&
                          fabsf(pd.headPos[1]-pd.rootPos[1]) < 0.001f &&
                          fabsf(pd.headPos[2]-pd.rootPos[2]) < 0.001f)) {
                    pd.headPos[1] += 3.5f;
                }

                if (cp.hasLimbs) {
                    pd.hasLimbs = true;
                    GetPartPositionCached(g_pid, cp.lFoot, pd.lFoot);
                    GetPartPositionCached(g_pid, cp.rFoot, pd.rFoot);
                    GetPartPositionCached(g_pid, cp.lHand, pd.lHand);
                    GetPartPositionCached(g_pid, cp.rHand, pd.rHand);
                }

                /* Health Reading */
                if (cp.humanoid) {
                    float h = Driver::Read<float>(g_pid, cp.humanoid + offsets::Humanoid::Health);
                    float mh = Driver::Read<float>(g_pid, cp.humanoid + offsets::Humanoid::MaxHealth);

                    /* FIX: maxHealth == 0 means the Humanoid is not fully initialised yet
                       (happens the first frame after a player joins / respawns).
                       Treat as full health and force a re-read next frame by clearing
                       the cached humanoid so FindChildByClass runs again. */
                    if (mh <= 0.0f) {
                        pd.health = 100.0f;
                        pd.maxHealth = 100.0f;
                        cp.humanoid = 0;   /* will be re-found next Update() call */
                    }
                    else {
                        /* Clamp health to [0, maxHealth] so a bad read never shows 0 HP
                           on a player who just loaded in. */
                        pd.maxHealth = mh;
                        pd.health = (h < 0.0f) ? 0.0f : (h > mh ? mh : h);
                    }
                }
                else {
                    /* Try to (re-)find the humanoid now that the character is present */
                    cp.humanoid = FindChildByClass(g_pid, cp.character, "Humanoid");

                    if (cp.humanoid) {
                        /* Got it this frame - read straight away */
                        float h = Driver::Read<float>(g_pid, cp.humanoid + offsets::Humanoid::Health);
                        float mh = Driver::Read<float>(g_pid, cp.humanoid + offsets::Humanoid::MaxHealth);
                        pd.maxHealth = (mh > 0.0f) ? mh : 100.0f;
                        pd.health = (mh > 0.0f && h >= 0.0f) ? (h > mh ? mh : h) : pd.maxHealth;
                    }
                    else {
                        /* Custom Health Logic (non-Humanoid rigs) */
                        uintptr_t healthVal = FindChildByName(g_pid, cp.character, "Health");
                        if (!healthVal) healthVal = FindChildByName(g_pid, cp.character, "health");
                        if (!healthVal) healthVal = FindChildByName(g_pid, cp.character, "hp");

                        if (healthVal) {
                            float h = Driver::Read<float>(g_pid, healthVal + offsets::Val::Value);
                            pd.health = (h > 0.0f) ? h : 100.0f;
                            pd.maxHealth = 100.0f;
                        }
                        else {
                            /* No health source found yet - assume alive until proven otherwise */
                            pd.health = 100.0f; pd.maxHealth = 100.0f;
                        }
                    }
                }

                g_players.push_back(pd);
            }
        }
    }

    /* ── Continuous per-frame writes ───────────────────────────── */
    Roblox::ApplyWorldSettings();
    Roblox::HitboxExpanderTick();
}

/* ════════════════════════════════════════════════════════════════
   WORLD SETTINGS / LIGHTING API
   ════════════════════════════════════════════════════════════════ */
namespace LightOff {
    static constexpr uintptr_t FogEnd       = offsets::Lighting::FogEnd;
    static constexpr uintptr_t FogStart     = offsets::Lighting::FogStart;
    static constexpr uintptr_t FogColor     = offsets::Lighting::FogColor;
    static constexpr uintptr_t Brightness   = offsets::Lighting::Brightness;
    static constexpr uintptr_t ClockTime    = offsets::Lighting::ClockTime;
    static constexpr uintptr_t Ambient      = offsets::Lighting::Ambient;
    static constexpr uintptr_t OutdoorAmb   = offsets::Lighting::OutdoorAmbient;
    static constexpr uintptr_t ExposureComp = offsets::Lighting::ExposureCompensation;
}

uintptr_t Roblox::GetLightingService() { return g_lightingService; }

void Roblox::ApplyWorldSettings() {
    if (!g_pid || !g_lightingService) return;
    const WorldSettings& w = g_cfg.world;
    if (w.fogEnabled) {
        Driver::Write<float>(g_pid, g_lightingService + LightOff::FogEnd,   w.fogEnd);
        Driver::Write<float>(g_pid, g_lightingService + LightOff::FogStart, w.fogStart);
        float fc[3] = {w.fogColor[0], w.fogColor[1], w.fogColor[2]};
        Driver::WriteRaw(g_pid, g_lightingService + LightOff::FogColor, fc, sizeof(fc));
    }
    if (w.lightingEnabled) {
        Driver::Write<float>(g_pid, g_lightingService + LightOff::Brightness,   w.brightness);
        Driver::Write<float>(g_pid, g_lightingService + LightOff::ClockTime,    w.clockTime);
        Driver::Write<float>(g_pid, g_lightingService + LightOff::ExposureComp, w.exposureComp);
        float amb[3]  = {w.ambient[0],        w.ambient[1],        w.ambient[2]};
        float oamb[3] = {w.outdoorAmbient[0], w.outdoorAmbient[1], w.outdoorAmbient[2]};
        Driver::WriteRaw(g_pid, g_lightingService + LightOff::Ambient,    amb,  sizeof(amb));
        Driver::WriteRaw(g_pid, g_lightingService + LightOff::OutdoorAmb, oamb, sizeof(oamb));
    }
}

void Roblox::SetFogEnd(float end)   { if(g_pid&&g_lightingService) Driver::Write<float>(g_pid,g_lightingService+LightOff::FogEnd,end); }
void Roblox::SetFogStart(float s)   { if(g_pid&&g_lightingService) Driver::Write<float>(g_pid,g_lightingService+LightOff::FogStart,s); }
void Roblox::SetFogColor(float r,float g2,float b) { if(!g_pid||!g_lightingService)return; float c[3]={r,g2,b}; Driver::WriteRaw(g_pid,g_lightingService+LightOff::FogColor,c,12); }
void Roblox::SetBrightness(float b) { if(g_pid&&g_lightingService) Driver::Write<float>(g_pid,g_lightingService+LightOff::Brightness,b); }
void Roblox::SetClockTime(float t)  { if(t<0)t=0;if(t>24)t=24; if(g_pid&&g_lightingService) Driver::Write<float>(g_pid,g_lightingService+LightOff::ClockTime,t); }
void Roblox::SetAmbient(float r,float g2,float b) { if(!g_pid||!g_lightingService)return; float c[3]={r,g2,b}; Driver::WriteRaw(g_pid,g_lightingService+LightOff::Ambient,c,12); }
void Roblox::SetOutdoorAmbient(float r,float g2,float b) { if(!g_pid||!g_lightingService)return; float c[3]={r,g2,b}; Driver::WriteRaw(g_pid,g_lightingService+LightOff::OutdoorAmb,c,12); }
void Roblox::SetExposureCompensation(float e) { if(e<-5)e=-5;if(e>5)e=5; if(g_pid&&g_lightingService) Driver::Write<float>(g_pid,g_lightingService+LightOff::ExposureComp,e); }

/* ════════════════════════════════════════════════════════════════
   HITBOX EXPANDER
   ════════════════════════════════════════════════════════════════
   Ports logic from hitbox_expander.cpp / silent.cpp:
   • When enabled: writes a larger Size vector (3 floats) to every
     enemy HumanoidRootPart primitive so bullets register easier.
   • When disabled: restores each part to the default R6/R15 size.
   • Team check: reads g_localPlayer's team and skips teammates.
   • CanCollide flag: written to PrimitiveFlags byte 0x02 on the
     primitive (bit 1 of the flags byte = CanCollide in Roblox).
   • Called from Roblox::Update() every frame (fast — only a few
     Driver::Write calls per enemy, no scanning).

   Primitive memory layout (at BasePart::Primitive ptr):
     +0x1B0  Size    Vector3 (3× float = 12 bytes)
     +0x1AE  PrimitiveFlags  uint16
                 bit 1 (0x02) = Anchored
                 bit 3 (0x08) = CanCollide  ← we write this

   Note on CanCollide: in Roblox r6 the CanCollide flag lives in
   PrimitiveFlags at byte offset 0x1AE.  We set/clear bit 0x08.
*/

static const float kDefaultHRPSize[3] = { 2.0f, 2.0f, 1.0f };

void Roblox::HitboxExpanderTick()
{
    if (!g_pid) return;
    const HitboxSettings& h = g_cfg.hitbox;

    /* Read local player ptr for team check */
    uintptr_t localPtr = 0;
    {
        std::lock_guard<std::mutex> lock(g_playerListMutex);
        for (const auto& cp : g_playerCache) {
            /* Find local by matching to our cached localplayer */
            if (cp.valid && cp.ptr) {
                /* We detect local by isLocalPlayer flag in g_players */
                break;
            }
        }
    }
    /* Use g_players to find local ptr */
    uintptr_t localPlayerPtr = 0;
    for (const auto& p : g_players) {
        if (p.isLocalPlayer) { localPlayerPtr = p.ptr; break; }
    }

    /* Helper: write size + canCollide to a primitive */
    auto WritePrimSize = [&](uintptr_t primitive, const float size[3], bool canCollide) {
        if (!primitive) return;
        Driver::WriteRaw(g_pid, primitive + offsets::Primitive::Size,
            (void*)size, 12);
        /* CanCollide lives in PrimitiveFlags (uint16) at +0x1AE.
         * Bit 0x08 = CanCollide.  Read → modify → write. */
        uint16_t flags = Driver::Read<uint16_t>(g_pid,
            primitive + offsets::Primitive::PrimitiveFlags);
        if (canCollide) flags |=  0x08;
        else            flags &= ~0x08;
        Driver::Write<uint16_t>(g_pid,
            primitive + offsets::Primitive::PrimitiveFlags, flags);
    };

    /* Build expanded size once */
    float expandedSize[3] = { h.sizeX, h.sizeY, h.sizeZ };

    std::lock_guard<std::mutex> lock(g_playerListMutex);

    if (!h.enabled) {
        /* Restore every enemy to default size */
        for (const auto& cp : g_playerCache) {
            if (!cp.valid || !cp.ptr) continue;
            if (cp.ptr == localPlayerPtr) continue;
            WritePrimSize(cp.rootPart.primitive, kDefaultHRPSize, true);
        }
        return;
    }

    /* Expand enemy hitboxes */
    for (const auto& cp : g_playerCache) {
        if (!cp.valid || !cp.ptr) continue;
        if (cp.ptr == localPlayerPtr) continue;

        /* Team check: read team pointers from the Player instance */
        if (h.teamCheck && localPlayerPtr) {
            uintptr_t localTeam  = Driver::Read<uintptr_t>(g_pid,
                localPlayerPtr + offsets::Player::Team);
            uintptr_t playerTeam = Driver::Read<uintptr_t>(g_pid,
                cp.ptr        + offsets::Player::Team);
            /* Same non-zero team = teammate, skip */
            if (localTeam && playerTeam && localTeam == playerTeam) continue;
        }

        if (!cp.rootPart.primitive) continue;

        WritePrimSize(cp.rootPart.primitive, expandedSize, h.canCollide);
    }
}

void Roblox::SetWalkSpeed(float speed) {
    if (!g_pid || !g_localHumanoid) return;
    Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkSpeed, speed);
    Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::WalkSpeedCheck, speed);
}

void Roblox::SetJumpPower(float power) {
    if (!g_pid || !g_localHumanoid) return;
    Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::JumpPower, power);
    Driver::Write<float>(g_pid, g_localHumanoid + offsets::Humanoid::JumpHeight, power);
}

uintptr_t Roblox::GetWorkspace() { return g_workspaceService; }

bool Roblox::GetLocalPlayerPos(float out[3]) {
    if (!g_pid || !g_localRootPart) return false;
    float cframe[12] = {};
    if (!Driver::ReadRaw(g_pid, g_localRootPart + offsets::Prim::CFrame, cframe, sizeof(cframe)))
        return false;
    out[0] = cframe[9]; out[1] = cframe[10]; out[2] = cframe[11];
    return true;
}

std::vector<PlayerData> Roblox::GetPlayersCopy() {
    std::lock_guard<std::mutex> lock(g_playerListMutex);
    return g_players;
}

/* ── Multithreaded Scanner ───────────────────────────── */

void ScannerLoop() {
    Console::Log("[Scanner] Thread Started.");
    while (g_scannerRunning) {
        if (!g_pid || !g_base) {
            Sleep(500);
            continue;
        }

        DWORD now = GetTickCount();

        /* ── Service Validity Check (1000ms) ────────── */
        static DWORD lastServiceCheck = 0;
        if (now - lastServiceCheck > 1000) {
            lastServiceCheck = now;

            bool servicesInvalid = false;
            if (g_playersService && GetRobloxClassName(g_pid, g_playersService) != "Players") servicesInvalid = true;
            if (g_workspaceService && GetRobloxClassName(g_pid, g_workspaceService) != "Workspace") servicesInvalid = true;

            if (servicesInvalid) {
                Console::Warn("[Scanner] Services Invalidated - Resetting pointers.");
                std::lock_guard<std::mutex> lock(g_playerListMutex);
                g_playersService = 0;
                g_workspaceService = 0;
                g_lightingService  = 0;
                g_playerCache.clear();
                g_idToName.clear();
            }
        }

        /* ── Update Debug Strings ────────────────────────────────── */
        if (now - g_lastDebugUpdate > 2000 && g_debugEnabled) {
            g_lastDebugUpdate = now;
            Console::Log("[Scanner] Players: %d | Entities: %d | Cache: %d",
                (int)g_players.size(),
                (int)(g_playerCache.size() - g_players.size()),
                (int)g_playerCache.size());

            /* detailed log of first 5 entities */
            int count = 0;
            for (const auto& p : g_playerCache) {
                if (count++ > 5) break;
                Console::Log("[Entity] Name: %s | Valid: %d | Root: %llX | Humanoid: %llX",
                    p.name.c_str(), p.valid, p.rootPart.ptr, p.humanoid);
            }

            /* WS SCAN: Inspect Workspace children to find character folders */
            if (g_workspaceService) {
                Console::Warn("--- INSPECTING WORKSPACE CHILDREN ---");
                auto kids = GetChildren(g_pid, g_workspaceService);
                int kCount = 0;
                for (auto k : kids) {
                    if (kCount++ > 40) break; // Limit spam, but want enough to see folders
                    uintptr_t namePtr = Driver::Read<uintptr_t>(g_pid, k + offsets::Instance::Name);
                    std::string kName = namePtr ? Driver::ReadRobloxString(g_pid, namePtr, 0) : "??";
                    std::string kClass = GetRobloxClassName(g_pid, k);
                    Console::Log("  > WS Child: %s | Class: %s", kName.c_str(), kClass.c_str());

                    /* DEEP DIVE into 'characters' folder CHECKING MODEL STRUCTURE */
                    if (kName == "characters" || kName == "Units") {
                        Console::Warn("    >>> INSPECTING %s FOLDER <<<", kName.c_str());
                        auto subKids = GetChildren(g_pid, k);
                        int sc = 0;
                        for (auto s : subKids) {
                            if (sc++ > 2) break; // First 2 items
                            uintptr_t snPtr = Driver::Read<uintptr_t>(g_pid, s + offsets::Instance::Name);
                            std::string snName = snPtr ? Driver::ReadRobloxString(g_pid, snPtr, 0) : "??";
                            std::string snClass = GetRobloxClassName(g_pid, s);
                            Console::Log("      > Item: %s | Class: %s", snName.c_str(), snClass.c_str());

                            /* Inspect Children of the Model */
                            if (snClass == "Model") {
                                auto itemKids = GetChildren(g_pid, s);
                                Console::Warn("        --- Model Structure ---");
                                int ic = 0;
                                for (auto ik : itemKids) {
                                    // if (ic++ > 20) break; // Log first 20 parts
                                    uintptr_t ikNamePtr = Driver::Read<uintptr_t>(g_pid, ik + offsets::Instance::Name);
                                    std::string ikName = ikNamePtr ? Driver::ReadRobloxString(g_pid, ikNamePtr, 0) : "??";
                                    std::string ikClass = GetRobloxClassName(g_pid, ik);
                                    Console::Log("        > Child: %s | Class: %s", ikName.c_str(), ikClass.c_str());
                                }
                                Console::Warn("        -----------------------");
                            }
                        }
                        Console::Warn("    >>> END FOLDER <<<");
                    }
                }
                Console::Warn("-------------------------------------");
            }
        }

        /* ── Low Frequency: Workspace Entities (250ms) ───────────── */
        if (now - g_lastEntityUpdate > 250) {
            g_lastEntityUpdate = now;

            if (!g_workspaceService && g_currentDataModel) {
                g_workspaceService = Driver::Read<uintptr_t>(g_pid, g_currentDataModel + offsets::DataModel::Workspace);
                if (g_workspaceService) {
                    Console::Success("[Scanner] Workspace found: 0x%llX", g_workspaceService);
                }
            }

            if (g_workspaceService) {
                /* 1. Snapshot ALL known addresses (ptr + character) to prevent duplicates */
                std::vector<uintptr_t> snapshot;
                {
                    std::lock_guard<std::mutex> lock(g_playerListMutex);
                    snapshot.reserve(g_playerCache.size() * 2);
                    for (const auto& p : g_playerCache) {
                        snapshot.push_back(p.ptr);
                        if (p.character && p.character != p.ptr)
                            snapshot.push_back(p.character);
                    }
                }

                /* 2. Scan for NEW entities */
                std::vector<CachedPlayer> newEntities;
                size_t totalCandidates = 0;

                auto ScanRecursive = [&](auto&& self, uintptr_t folder, int depth, bool isTargetFolder) -> void {
                    if (depth > 10 || totalCandidates > 100000) return; // Scale for PF Maps
                    auto children = GetChildren(g_pid, folder);

                    for (auto child : children) {
                        if (totalCandidates > 100000) return;

                        uintptr_t namePtr = Driver::Read<uintptr_t>(g_pid, child + offsets::Instance::Name);
                        std::string n = (namePtr) ? Driver::ReadRobloxString(g_pid, namePtr, 0) : "Unnamed";
                        std::string cls = GetRobloxClassName(g_pid, child);

                        /* Aggressive Blacklist (Skip map/effect geometry) */
                        if (!isTargetFolder && depth < 4) {
                            if (n == "Camera" || n == "Terrain" || n == "Baseplate" ||
                                n == "Debris" || n == "RobloxReplicatedStorage" ||
                                n == "Clouds" || n == "SoundService" || n == "Lighting" ||
                                n == "CoreGui" || n == "Chat" || n == "Geometry" ||
                                n == "Map" || n == "Scenery" || n == "ScriptContext" ||
                                n == "Environment" || n == "Level" || n == "Static" ||
                                n == "Props" || n == "Vegetation" || n == "Roads" ||
                                n == "Buildings" || n == "Bricks" || n == "Floor" || n == "Wall" ||
                                /* PF-specific: skip effect/weapon/spawn containers */
                                n == "Misc" || n == "GunDrop" || n == "Blood" ||
                                n == "Bullets" || n == "DeadBody" || n == "Roots" ||
                                n == "GameModeProps" || n == "MapParts" || n == "Spawns" ||
                                n == "Sound")
                                continue;
                        }

                        if (cls == "Model") {
                            /* === Workspace Rig Detection (Tier 3 ONLY) ===
                               Real players are tracked by the Players service.
                               NPCs have identical structure (HRP, Head, Torso) so
                               name-based checks CANNOT distinguish them from players.
                               We ONLY use the class-based heuristic for PF obfuscated rigs. */

                               /* Skip known non-player entities by name UNLESS in target folder */
                            if (!isTargetFolder) {
                                if (n.find("deadbody") != std::string::npos ||
                                    n.find("DeadBody") != std::string::npos ||
                                    n.find("Dummy") != std::string::npos ||
                                    n.find("NPC") != std::string::npos ||
                                    n.find("Corpse") != std::string::npos) continue;
                            }

                            uintptr_t humanoid = FindChildByClass(g_pid, child, "Humanoid");
                            bool isPotentialRig = false;

                            /* Tier 3: Class-based heuristic (PF obfuscated names)
                               Characters: 6-60 children, 5+ Part/MeshPart,
                               structural signature: root-like (has joints) + limb-like (no children).
                               MUST HAVE A HUMANOID to be a character (filters props/vests). */
                            if (humanoid) {
                                if (isTargetFolder) {
                                    // Whitelisted folder -> Relax strict checks
                                    isPotentialRig = true;
                                }
                                else {
                                    // Strict checks for random models
                                    auto modelChildren = GetChildren(g_pid, child);
                                    size_t totalC = modelChildren.size();
                                    if (totalC >= 6 && totalC <= 60) {
                                        int partCount = 0;
                                        int partsWithChildren = 0;
                                        int partsNoChildren = 0;
                                        for (auto mc : modelChildren) {
                                            std::string mcCls = GetRobloxClassName(g_pid, mc);
                                            if (mcCls == "Part" || mcCls == "MeshPart") {
                                                partCount++;
                                                auto partCh = GetChildren(g_pid, mc);
                                                if (partCh.size() >= 3) partsWithChildren++;
                                                else if (partCh.size() == 0) partsNoChildren++;
                                            }
                                        }
                                    }
                                }
                            }

                            /* Whitelisted folder -> Accept ANY model regardless of Humanoid */
                            if (isTargetFolder) {
                                isPotentialRig = true;
                            }

                            if (isPotentialRig) {
                                /* Check against snapshot AND already-found entities */
                                bool exists = false;
                                for (uintptr_t s : snapshot) if (s == child) { exists = true; break; }
                                if (!exists) {
                                    for (const auto& ne : newEntities) if (ne.ptr == child) { exists = true; break; }
                                }

                                if (!exists) {
                                    auto rigChildren = GetChildren(g_pid, child);
                                    if (rigChildren.size() > 200) continue;

                                    CachedPlayer newP = {};
                                    newP.ptr = child;
                                    newP.character = child;
                                    newP.humanoid = humanoid;
                                    newP.isEntity = true;
                                    newP.name = n;
                                    Console::Success("[Scanner] Entity Found: '%s' (In Target: %d)", n.c_str(), isTargetFolder);

                                    // ID Resolution
                                    if (!newP.name.empty() && isdigit(newP.name[0])) {
                                        try {
                                            long long id = std::stoll(newP.name);
                                            {
                                                std::lock_guard<std::mutex> lock(g_playerListMutex);
                                                if (g_idToName.count(id)) newP.name = g_idToName[id];
                                            }
                                        }
                                        catch (...) {}
                                    }

                                    newEntities.push_back(newP);
                                    snapshot.push_back(child);
                                }
                            }
                            /* Do NOT recurse into Models - only Folders contain player characters.
                               Recursing into Models finds NPCs inside map geometry. */
                            totalCandidates++;
                        }
                        else if (cls == "Folder") {
                            // Check for whitelist folders
                            bool nextIsTarget = isTargetFolder || (n == "characters" || n == "Players" || n == "Units");
                            self(self, child, depth + 1, nextIsTarget);
                        }
                    }
                    };

                // Initial Call
                ScanRecursive(ScanRecursive, g_workspaceService, 0, false);

                /* Periodic Heartbeat Log */
                static DWORD lastHeartbeat = 0;
                if (now - lastHeartbeat > 5000) {
                    lastHeartbeat = now;
                    size_t cacheCount = 0;
                    { std::lock_guard<std::mutex> lock(g_playerListMutex); cacheCount = g_playerCache.size(); }
                    Console::Log("[Scanner] Cycle complete: %d items scanned, %d in cache, %d new found.", totalCandidates, cacheCount, newEntities.size());
                }

                /* 3. Merge new entities (check both ptr AND character to prevent duplicates) */
                if (!newEntities.empty()) {
                    std::lock_guard<std::mutex> lock(g_playerListMutex);
                    for (const auto& p : newEntities) {
                        bool exists = false;
                        for (const auto& cp : g_playerCache) {
                            if (cp.ptr == p.ptr) { exists = true; break; }
                            // Also check if this entity's character matches an existing player's character
                            if (cp.character && cp.character == p.character) { exists = true; break; }
                        }
                        if (!exists) g_playerCache.push_back(p);
                    }
                }
            }
        }

        /* ── Update Player Data (Standard Players) ── */
        static DWORD lastPlayerCacheUpdate = 0;
        if (now - lastPlayerCacheUpdate > 200) {
            lastPlayerCacheUpdate = now;

            if (!g_playersService && g_currentDataModel) g_playersService = FindChildByClass(g_pid, g_currentDataModel, "Players");

            if (g_playersService) {
                std::vector<uintptr_t> playerChildren = GetChildren(g_pid, g_playersService);

                /* Remove Invalid Checks (Needs Lock) */
                {
                    std::lock_guard<std::mutex> lock(g_playerListMutex);
                    for (auto it = g_playerCache.begin(); it != g_playerCache.end();) {
                        if (!it->isEntity) {
                            // Players service: remove if player left
                            bool found = false;
                            for (auto child : playerChildren) {
                                if (child == it->ptr) { found = true; break; }
                            }
                            if (!found) {
                                it = g_playerCache.erase(it);
                                continue;
                            }
                        }
                        else {
                            // Workspace entities: remove if Model is destroyed (0 children)
                            auto entChildren = GetChildren(g_pid, it->ptr);
                            if (entChildren.empty()) {
                                it = g_playerCache.erase(it);
                                continue;
                            }
                        }
                        ++it;
                    }
                }

                /* Add New Players */
                for (auto child : playerChildren) {
                    // We can do classname check without lock
                    if (GetRobloxClassName(g_pid, child) == "Player") {
                        bool known = false;
                        {
                            std::lock_guard<std::mutex> lock(g_playerListMutex);
                            for (const auto& cp : g_playerCache) if (cp.ptr == child) { known = true; break; }
                        }

                        if (!known) {
                            uintptr_t namePtr = Driver::Read<uintptr_t>(g_pid, child + offsets::Instance::Name);
                            std::string pName = (namePtr) ? Driver::ReadRobloxString(g_pid, namePtr, 0) : "Unknown";

                            /* DisplayName is a separate property on the Player instance.
                               It sits at offsets::Player::DisplayName (Roblox string ptr). */
                            std::string pDisplayName;
                            uintptr_t dispPtr = Driver::Read<uintptr_t>(g_pid, child + offsets::Player::DisplayName);
                            if (dispPtr) pDisplayName = Driver::ReadRobloxString(g_pid, dispPtr, 0);
                            if (pDisplayName.empty()) pDisplayName = pName; /* fallback to username */

                            long long userId = Driver::Read<long long>(g_pid, child + offsets::Player::UserId);

                            // Update global map (needs lock? Map is only used here and in recursive which is same thread)
                            // Actually map is read in recursive which is same thread. So no lock needed for map.
                            if (userId > 0 && !pName.empty()) {
                                std::lock_guard<std::mutex> lock(g_playerListMutex);
                                g_idToName[userId] = pName;
                            }

                            CachedPlayer newP = {};
                            newP.ptr = child;
                            newP.isEntity = false;
                            newP.name = pName;
                            newP.displayName = pDisplayName;

                            std::lock_guard<std::mutex> lock(g_playerListMutex);
                            g_playerCache.push_back(newP);
                        }
                    }
                }
            }
        }

        Sleep(50);
    }
}

void Roblox::StartScanner() {
    if (g_scannerRunning) return;
    g_scannerRunning = true;
    g_scannerThread = std::thread(ScannerLoop);
    g_scannerThread.detach(); // Detach to run independently
}

void Roblox::StopScanner() {
    g_scannerRunning = false;
    // Thread will exit on next loop, detached
}


DWORD Roblox::GetPID() { return g_pid; }
uintptr_t Roblox::GetBase() { return g_base; }
const float* Roblox::GetViewMatrix() { return g_viewMatrix; }
const std::vector<PlayerData>& Roblox::GetPlayers() {
    // Mutex should be here but for read-heavy ESP it might flicker.
    // For now returning the vector is safe-ish if we don't clear/resize during read.
    // But better to return a COPY if possible or use mutex.
    // Let's rely on atomic/fast updates for now to avoid locking render thread.
    return g_players;
}
int Roblox::GetScreenWidth() { return g_screenW; }
int Roblox::GetScreenHeight() { return g_screenH; }
HWND Roblox::GetWindow() { return g_hwnd; }

/* ── Camera helpers ──────────────────────────────────────────────
 * Roblox Camera CFrame layout (48 bytes at CameraRotation offset):
 *   [0..8]  = 3x3 rotation matrix, row-major (right, up, -look)
 *   [9..11] = position (X, Y, Z)
 * We read this, rotate by dPitch/dYaw, and write it back.         */

static uintptr_t GetCameraPtr()
{
    if (!g_workspaceService) return 0;
    /* Workspace + offsets::CameraInstance -> CurrentCamera instance ptr */
    uintptr_t cam = Driver::Read<uintptr_t>(g_pid, g_workspaceService + offsets::CameraInstance);
    return cam;
}

/* Roblox CFrame memory layout (12 floats, column-major 3x3 + position):
 *   cf[0]  = right.X    cf[1]  = right.Y    cf[2]  = right.Z
 *   cf[3]  = up.X       cf[4]  = up.Y       cf[5]  = up.Z
 *   cf[6]  = back.X     cf[7]  = back.Y     cf[8]  = back.Z
 *   cf[9]  = pos.X      cf[10] = pos.Y      cf[11] = pos.Z
 *
 * "back" = negative look direction (Roblox convention).
 * Extract current yaw/pitch from back vector, apply deltas, clamp,
 * then rebuild a clean orthonormal matrix — no drift, no flip.     */
static void RotateCFrame(float cf[12], float dPitchDeg, float dYawDeg)
{
    const float kPi    = 3.14159265358979323846f;
    const float toRad  = kPi / 180.f;

    /* Extract yaw and pitch from the back vector (cf[6..8]) */
    /* back.X = cf[6], back.Y = cf[7], back.Z = cf[8]        */
    float yaw   = atan2f(cf[6], cf[8]);   /* horizontal: atan2(back.X, back.Z) */
    float pitch = asinf( cf[7]);           /* vertical:   asin(back.Y)          */
    /*   note: back = -look, so back.Y = sin(pitch) directly  */

    /* Apply deltas */
    yaw   += dYawDeg   * toRad;
    pitch += dPitchDeg * toRad;

    /* Clamp pitch ±89° */
    const float kMax = 89.f * toRad;
    if (pitch >  kMax) pitch =  kMax;
    if (pitch < -kMax) pitch = -kMax;

    /* Rebuild axes from yaw + pitch (no roll) */
    float sy = sinf(yaw),  cy = cosf(yaw);
    float sp = sinf(pitch), cp = cosf(pitch);

    /* back  = ( sy*cp,  sp, -cy*cp )   [negative look] */
    /* right = ( cy,     0,   sy    )   [flat, yaw only] */
    /* up    = cross(right, -back)  → computed below     */

    float bx =  sy*cp,  by = sp,   bz = -cy*cp;
    float rx =  cy,     ry = 0.f,  rz =  sy;

    /* up = cross(right, -back) = cross(right, look) */
    /* look = -back = (-bx, -by, -bz)                */
    float lx = -bx, ly = -by, lz = -bz;
    float ux = ry*lz - rz*ly;
    float uy = rz*lx - rx*lz;
    float uz = rx*ly - ry*lx;
    /* normalise up */
    float uLen = sqrtf(ux*ux + uy*uy + uz*uz);
    if (uLen < 1e-6f) return;
    ux /= uLen; uy /= uLen; uz /= uLen;

    /* Write back — column order: right, up, back, position untouched */
    cf[0]=rx; cf[1]=ry; cf[2]=rz;
    cf[3]=ux; cf[4]=uy; cf[5]=uz;
    cf[6]=bx; cf[7]=by; cf[8]=bz;
    /* cf[9..11] unchanged */
}

void Roblox::AdjustCameraAngles(float yawRad, float pitchRad)
{
    if (!g_pid) return;
    uintptr_t cam = GetCameraPtr();
    if (!cam) return;

    /* Build a clean orthonormal CFrame from absolute yaw + pitch.
     * No reading back from memory — avoids all layout/drift bugs.
     *
     * Roblox CFrame layout (column-major 3x3 + position, 12 floats):
     *   cf[0..2]  = right  vector (X,Y,Z)
     *   cf[3..5]  = up     vector (X,Y,Z)
     *   cf[6..8]  = back   vector (X,Y,Z)  (-look)
     *   cf[9..11] = position
     */
    float cf[12] = {};
    /* Read full CFrame — position (cf[9..11]) will be preserved in write */
    if (!Driver::ReadRaw(g_pid, cam + offsets::CameraRotation, cf, sizeof(cf))) return;
    /* Sanity: if position is all zero the camera ptr is probably wrong */
    if (cf[9] == 0.f && cf[10] == 0.f && cf[11] == 0.f) return;

    const float kM = 89.f * (float)(3.14159265358979323846 / 180.0);
    if (pitchRad >  kM) pitchRad =  kM;
    if (pitchRad < -kM) pitchRad = -kM;

    float sy = sinf(yawRad),   cy = cosf(yawRad);
    float sp = sinf(pitchRad), cp = cosf(pitchRad);

    /* back  = ( sy*cp,  sp, -cy*cp )   (negative look) */
    /* right = ( cy,     0,   sy    )   (no roll)        */
    /* up    = cross(right, look)       where look = -back */
    float bx =  sy*cp, by = sp,   bz = -cy*cp;
    float rx =  cy,    ry = 0.f,  rz =  sy;
    float lx = -bx,   ly = -by,  lz = -bz;   /* look = -back */

    float ux = ry*lz - rz*ly;
    float uy = rz*lx - rx*lz;
    float uz = rx*ly - ry*lx;
    float uLen = sqrtf(ux*ux + uy*uy + uz*uz);
    if (uLen < 1e-6f) return;
    ux/=uLen; uy/=uLen; uz/=uLen;

    cf[0]=rx; cf[1]=ry; cf[2]=rz;
    cf[3]=ux; cf[4]=uy; cf[5]=uz;
    cf[6]=bx; cf[7]=by; cf[8]=bz;
    /* cf[9..11] = position, already read, unchanged */

    Driver::WriteRaw(g_pid, cam + offsets::CameraRotation, cf, sizeof(cf));
}

void Roblox::SetLocalYaw(float yawDeg)
{
    if (!g_pid || !g_localRootPart) return;

    float cf[12] = {};
    if (!Driver::ReadRaw(g_pid, g_localRootPart + offsets::Prim::CFrame, cf, sizeof(cf))) return;

    /* Overwrite rotation with pure yaw around world-Y, preserving position */
    float y = yawDeg * (float)(3.14159265358979323846 / 180.0);
    float cy = cosf(y), sy = sinf(y);
    /* right=(cy,0,sy)  up=(0,1,0)  look=(-sy,0,cy) */
    cf[0]=cy;  cf[1]=0.f; cf[2]=sy;
    cf[3]=0.f; cf[4]=1.f; cf[5]=0.f;
    cf[6]=-sy; cf[7]=0.f; cf[8]=cy;
    /* cf[9..11] unchanged (position) */

    Driver::WriteRaw(g_pid, g_localRootPart + offsets::Prim::CFrame, cf, sizeof(cf));
}

void Roblox::SetLocalPitch(float pitchDeg)
{
    if (!g_pid || !g_localRootPart) return;

    float cf[12] = {};
    if (!Driver::ReadRaw(g_pid, g_localRootPart + offsets::Prim::CFrame, cf, sizeof(cf))) return;

    float p = pitchDeg * (float)(3.14159265358979323846 / 180.0);
    float cp = cosf(p), sp = sinf(p);
    /* Apply pitch around local-right axis (first row of current matrix) */
    float rx[3] = { cf[0], cf[1], cf[2] };
    float ry[3] = { cf[3], cf[4], cf[5] };
    float rz[3] = { cf[6], cf[7], cf[8] };
    for (int i = 0; i < 3; i++) {
        cf[3+i] = ry[i]*cp - rz[i]*sp;
        cf[6+i] = ry[i]*sp + rz[i]*cp;
    }
    (void)rx; /* right axis unchanged */

    Driver::WriteRaw(g_pid, g_localRootPart + offsets::Prim::CFrame, cf, sizeof(cf));
}
/* settings.cpp provides Config g_cfg */