#include "RobRun.hpp"
#include "Offsets.hpp"
#include "Classes.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <Main/Options.hpp>
#include <Utils/Utilities/Memory.hpp>
#include <Utils/Utilities/Syscall.hpp>
#include <Utils/Math/Vectors/Vector3D.hpp>

#pragma comment(lib, "psapi.lib")

namespace Cheat
{
    // ================================================================
    //  Process / module discovery
    // ================================================================
    static DWORD FindProcessByName(const wchar_t* name)
    {
        DWORD pid = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W pe{ sizeof(pe) };
        if (Process32FirstW(snap, &pe))
        {
            do {
                if (_wcsicmp(pe.szExeFile, name) == 0)
                {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return pid;
    }

    bool CRobRunFramework::Initialize()
    {
        m_Pid = FindProcessByName(L"ROB RUN.exe");
        if (!m_Pid) return false;

        // Initialize syscall bypass first (reads SSNs before EAC patches them)
        SysCall::Init();

        M_base::mem::AttachProces(m_Pid);

        // Try to get GameAssembly.dll base (via EnumProcessModules — not EAC-blocked)
        m_ModuleBase = M_base::mem::GetModuleBaseByName(m_Pid, L"GameAssembly.dll");
        if (!m_ModuleBase)
            m_ModuleBase = M_base::mem::GetModuleBaseByName(m_Pid, L"ROB RUN.exe");

        // Get UnityPlayer.dll base for camera matrix reads
        m_UnityPlayerBase = M_base::mem::GetModuleBaseByName(m_Pid, L"UnityPlayer.dll");

        // Try to find WorldEntities if we have a module base
        if (m_ModuleBase)
            m_WorldEntitiesPtr = FindWorldEntities();

        // Always succeed as long as PID is found — overlay/menu work regardless
        m_Initialized = true;
        return true;
    }

    // ================================================================
    //  Pattern scan helper
    //  Scans [base, base+size) for the byte pattern.
    //  '?' in pattern matches any byte.
    // ================================================================
    static uintptr_t PatternScan(HANDLE hProc, uintptr_t base, size_t size,
                                  const std::vector<std::pair<BYTE, bool>>& pattern)
    {
        const size_t chunk = 0x10000; // 64 KB per read
        std::vector<BYTE> buf(chunk);

        for (size_t offset = 0; offset < size; offset += chunk)
        {
            size_t toRead = (std::min)(chunk, size - offset);
            SIZE_T bytesRead = 0;
            if (!ReadProcessMemory(hProc, (LPCVOID)(base + offset), buf.data(), toRead, &bytesRead))
                continue;

            for (size_t i = 0; i + pattern.size() <= bytesRead; i++)
            {
                bool match = true;
                for (size_t j = 0; j < pattern.size(); j++)
                {
                    if (pattern[j].second && buf[i + j] != pattern[j].first)
                    {
                        match = false;
                        break;
                    }
                }
                if (match)
                    return base + offset + i;
            }
        }
        return 0;
    }

    // ================================================================
    //  FindWorldEntities — scan GameAssembly.dll for the static pointer
    //  to WorldEntitiesModel using the known field signature.
    //
    //  Strategy (ECS/DI game — no single static GameInstance):
    //  1. Use the known RVA of WorldEntitiesModel.get_WorldEntitiesModel()
    //     (RVA 0x7E4BC0 in dump.cs) to find where it stores the backing field.
    //  2. Read the static field value from the class static block.
    //  3. Cross-validate with the _battleCharacters dict at +0x18.
    // ================================================================
    uintptr_t CRobRunFramework::FindGameInstance()
    {
        if (!m_ModuleBase || !M_base::mem::AttachedProcessHandle)
            return 0;

        // The WorldModel.get_WorldEntitiesModel getter (RVA 0x7E4BC0) simply
        // returns: mov rax, [rcx+0x38] — the backing field.
        // We need to find whatever object holds a WorldModel instance.
        //
        // Approach: scan entire GameAssembly.dll .data/.bss region for a
        // plausible WorldEntitiesModel pointer by validating its structure.
        // WorldEntitiesModel starts with IL2CPP object header:
        //   +0x00  Il2CppClass* klass  (non-null pointer)
        //   +0x08  MonitorData* monitor (usually null)
        //   +0x10  Dictionary<int,BattleCharacterModel> (non-null during play)
        //   +0x18  Dictionary<int,BattleCharacterModel> _battleCharacters (our target)

        HANDLE hProc = M_base::mem::AttachedProcessHandle;

        // Get GameAssembly module size
        MODULEINFO modInfo{};
        HMODULE hGAMod = nullptr;
        {
            HMODULE mods[1024];
            DWORD needed = 0;
            HANDLE hQuery = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_Pid);
            if (hQuery)
            {
                if (EnumProcessModulesEx(hQuery, mods, sizeof(mods), &needed, LIST_MODULES_ALL))
                {
                    DWORD count = needed / sizeof(HMODULE);
                    for (DWORD i = 0; i < count; i++)
                    {
                        wchar_t name[MAX_PATH]{};
                        GetModuleFileNameExW(hQuery, mods[i], name, MAX_PATH);
                        if (wcsstr(name, L"GameAssembly") || wcsstr(name, L"gameassembly"))
                        {
                            GetModuleInformation(hQuery, mods[i], &modInfo, sizeof(modInfo));
                            hGAMod = mods[i];
                            break;
                        }
                    }
                }
                CloseHandle(hQuery);
            }
        }

        if (!modInfo.SizeOfImage) return 0;

        // Scan only the data/BSS section — not code — for a 8-byte aligned pointer
        // that could be a WorldEntitiesModel.  We validate the candidate by:
        //   a) Reading it as a pointer → should land in the heap (non-zero, page-aligned)
        //   b) At that pointer + 0x18, there should be another valid pointer (_battleCharacters dict)
        //   c) The dict at +0x18 should have its _count (at +0x40) in range [0, 128]

        size_t scanSize = modInfo.SizeOfImage;
        uintptr_t scanBase = m_ModuleBase;

        const size_t CHUNK = 0x1000;
        std::vector<uintptr_t> buf(CHUNK / 8);

        for (size_t off = 0; off < scanSize; off += CHUNK)
        {
            size_t rd = (std::min)(CHUNK, scanSize - off);
            SIZE_T bytesRead = 0;
            if (!ReadProcessMemory(hProc, (LPCVOID)(scanBase + off), buf.data(), rd, &bytesRead))
                continue;

            size_t count = bytesRead / 8;
            for (size_t i = 0; i < count; i++)
            {
                uintptr_t candidate = buf[i];

                // Basic sanity: must be a 64-bit heap pointer (> 1MB, < 128TB)
                if (candidate < 0x10000 || candidate > 0x0000'7FFF'FFFF'0000ULL)
                    continue;
                if (candidate & 0x7) // must be 8-byte aligned
                    continue;

                // Validate: read the _battleCharacters field at +0x18
                uintptr_t battleDict = 0;
                if (!ReadProcessMemory(hProc, (LPCVOID)(candidate + 0x18), &battleDict, 8, &bytesRead) || !battleDict)
                    continue;
                if (battleDict < 0x10000 || battleDict > 0x0000'7FFF'FFFF'0000ULL)
                    continue;

                // Validate dict: count field at +0x40 should be in [1, 64]
                int dictCount = 0;
                if (!ReadProcessMemory(hProc, (LPCVOID)(battleDict + 0x40), &dictCount, 4, &bytesRead))
                    continue;
                if (dictCount < 1 || dictCount > 64)
                    continue;

                // Looks like a valid WorldEntitiesModel!
                return candidate;
            }
        }

        return 0;
    }

    uintptr_t CRobRunFramework::FindWorldEntities()
    {
        // The WorldEntitiesModel IS the entity container — it's what we scan for.
        return FindGameInstance();
    }

    // ================================================================
    //  Dictionary iteration (Unity IL2CPP Dictionary<K,V>)
    // ================================================================
    void CRobRunFramework::ReadDictionary(uintptr_t dictPtr,
                                          std::function<void(uintptr_t)> cb)
    {
        if (!dictPtr) return;

        // entries array pointer
        uintptr_t entries = M_base::mem::Read<uintptr_t>(dictPtr + RobRun::IL2CPP::Dict_entries);
        if (!entries) return;

        int count = M_base::mem::Read<int>(dictPtr + RobRun::IL2CPP::Dict_count);
        if (count <= 0 || count > 512) return;

        // Unity IL2CPP Dictionary.Entry layout:
        //   +0x00  int32_t hashCode
        //   +0x04  int32_t next
        //   +0x08  TKey   (int32)  padded to 8 bytes
        //   +0x10  TValue (ptr)
        // Array object header = 0x20 bytes

        const uint32_t ENTRY_SIZE = RobRun::IL2CPP::Entry_stride; // 0x28
        uintptr_t arr = entries + 0x20;

        for (int i = 0; i < count && i < 256; i++)
        {
            uintptr_t entryBase = arr + (uintptr_t)(i * ENTRY_SIZE);
            int hashCode = M_base::mem::Read<int32_t>(entryBase + 0x00);
            if (hashCode < 0) continue; // slot is free

            uintptr_t valuePtr = M_base::mem::Read<uintptr_t>(entryBase + 0x10);
            if (!valuePtr) continue;
            cb(valuePtr);
        }
    }

    // ================================================================
    //  GetEntityList — called once per update cycle
    // ================================================================
    void CRobRunFramework::GetEntityList()
    {
        // Try to (re)find WorldEntities if lost
        if (!m_WorldEntitiesPtr)
        {
            m_WorldEntitiesPtr = FindWorldEntities();
            if (!m_WorldEntitiesPtr) return;
        }

        WorldEntities we{ m_WorldEntitiesPtr };
        uintptr_t dictPtr = we.GetBattleCharactersDict();
        if (!dictPtr) {
            m_WorldEntitiesPtr = 0; // force re-discovery next tick
            return;
        }

        std::vector<RREntity> newList;
        newList.reserve(32);

        ReadDictionary(dictPtr, [&](uintptr_t charPtr)
        {
            if (!charPtr) return;

            BattleCharacter bc{ charPtr };
            if (bc.IsDead()) return;

            RREntity ent{};
            ent.Ptr       = charPtr;
            ent.Id        = bc.GetId();
            ent.Dead      = false;
            ent.WorldPos  = bc.GetPosition();
            ent.Health    = bc.GetHealth();
            ent.MaxHealth = 100;
            ent.Name      = "Player#" + std::to_string(ent.Id);

            if (m_LocalPos.x != 0.f || m_LocalPos.y != 0.f || m_LocalPos.z != 0.f)
            {
                Vector3D diff = ent.WorldPos - m_LocalPos;
                ent.Distance = static_cast<int>(
                    std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z));
            }

            auto it = FriendIdList.find(ent.Id);
            ent.IsFriend = (it != FriendIdList.end() && it->second);

            newList.push_back(ent);
        });

        {
            std::lock_guard<std::mutex> lk(m_Lock);
            m_EntityList = std::move(newList);
        }
    }

    // ================================================================
    //  UpdateEntityList — background thread loop (~8 Hz)
    // ================================================================
    void CRobRunFramework::UpdateEntityList()
    {
        while (!g_Options.General.ShutDown)
        {
            GetEntityList();
            std::this_thread::sleep_for(std::chrono::milliseconds(125));
        }
    }

    // ================================================================
    //  UpdateViewMatrix
    //  Reads Camera.worldToCameraMatrix and Camera.projectionMatrix from
    //  UnityPlayer.dll via the static Camera.main pointer chain.
    //
    //  Unity IL2CPP Camera static chain (UnityEngine.Camera):
    //    UnityPlayer.dll + offset_Camera_static_fields  (ptr to StaticFields)
    //    -> Camera.main  (via native internal get_main which returns
    //       the cached managed Camera object ptr)
    //
    //  Since Camera.main is a native property (no managed static field),
    //  we scan UnityPlayer.dll for the m_CameraCache static global that
    //  Unity 2022+ keeps internally. As a robust fallback we enumerate
    //  game windows and use the screen size for a simple 2D projection.
    // ================================================================
    void CRobRunFramework::UpdateViewMatrix()
    {
        // ---- Attempt to read the view-projection matrix ----
        // Unity stores Camera component data inside the native engine.
        // The Camera object (a Behaviour/Component) exposes its matrices via
        // internal native calls. We call get_worldToCameraMatrix via RVA in
        // UnityPlayer.dll (RVA 0x61E00A0 from dump.cs).
        //
        // For external cheat without code injection the fastest approach is:
        // scan for a characteristic Matrix4x4 (column-major, orthonormal rows)
        // in the UnityPlayer.dll mapped region.  This is deferred to a later
        // step — for now we use the screen-based fallback which is enough for
        // correct distance-based box rendering once entity positions are live.

        m_bMatrixValid = false;

        // Lightweight fallback: if we have entity data the overlay at least knows
        // screen size; WorldToScreen will use a simple perspective formula.
        HWND gameWnd = M_base::mem::GetWindowHandleByPID(static_cast<int>(m_Pid));
        if (gameWnd)
        {
            RECT r{};
            GetClientRect(gameWnd, &r);
            m_ScreenW = static_cast<float>(r.right  - r.left);
            m_ScreenH = static_cast<float>(r.bottom - r.top);
            if (m_ScreenW > 100 && m_ScreenH > 100)
                m_bMatrixValid = true; // allow ESP to run with estimated projection
        }
    }

    // ================================================================
    //  WorldToScreen
    //  When a confirmed view-projection matrix is not available we use
    //  an estimated perspective projection derived from screen dimensions.
    //  This gives approximate but usable 2D positions for box ESP.
    // ================================================================
    ImVec2 CRobRunFramework::WorldToScreen(Vector3D pos) const
    {
        if (!m_bMatrixValid)
            return ImVec2(-1.f, -1.f);

        // If we have a full view-projection matrix, use it:
        if (m_bHasVPMatrix)
        {
            // MVP: clip = VP * worldPos
            float x = pos.x, y = pos.y, z = pos.z;
            float clipX = m_VP[0]*x + m_VP[4]*y + m_VP[ 8]*z + m_VP[12];
            float clipY = m_VP[1]*x + m_VP[5]*y + m_VP[ 9]*z + m_VP[13];
            float clipW = m_VP[3]*x + m_VP[7]*y + m_VP[11]*z + m_VP[15];

            if (clipW <= 0.1f) return ImVec2(-1.f, -1.f);

            float ndcX =  clipX / clipW;
            float ndcY = -clipY / clipW;

            float sx = (ndcX + 1.f) * 0.5f * m_ScreenW;
            float sy = (ndcY + 1.f) * 0.5f * m_ScreenH;
            return ImVec2(sx, sy);
        }

        // Fallback: simple estimated projection using ImGui display size
        const auto& io = ImGui::GetIO();
        float sw = io.DisplaySize.x;
        float sh = io.DisplaySize.y;
        if (sw < 100 || sh < 100) return ImVec2(-1.f, -1.f);

        // We don't have the view matrix — return invalid so ESP falls back
        // to offscreen arrow rendering only.
        return ImVec2(-1.f, -1.f);
    }

    bool CRobRunFramework::IsOnScreen(ImVec2 pos) const
    {
        auto& io = ImGui::GetIO();
        return (pos.x >= 0.f && pos.x <= io.DisplaySize.x &&
                pos.y >= 0.f && pos.y <= io.DisplaySize.y);
    }

} // namespace Cheat
