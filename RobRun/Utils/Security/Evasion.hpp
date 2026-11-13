#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <intrin.h>
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>
#include <Utils/Render/kaanlar.h>         // xorstr_
// NOTE: winternl.h intentionally excluded — its PEB stub is incomplete.
// All PEB fields are accessed via documented raw byte offsets.

#pragma comment(lib, "ntdll.lib")

namespace Security
{
    // -----------------------------------------------------------------------
    // 1. PE HEADER WIPE
    //    Zeros the MZ/PE header of our own module in memory.
    //    Prevents memory dumpers and scanners from reading the binary header.
    // -----------------------------------------------------------------------
    inline void WipePEHeader()
    {
        HMODULE hMod = GetModuleHandle(nullptr);
        if (!hMod) return;

        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;

        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(
            reinterpret_cast<uint8_t*>(hMod) + dos->e_lfanew);

        // Make header page writable
        DWORD oldProtect = 0;
        const SIZE_T headerSize = nt->OptionalHeader.SizeOfHeaders;
        VirtualProtect(hMod, headerSize, PAGE_READWRITE, &oldProtect);

        // Zero out MZ + PE headers entirely
        SecureZeroMemory(hMod, headerSize);

        // Restore original protection
        VirtualProtect(hMod, headerSize, oldProtect, &oldProtect);
    }

    // -----------------------------------------------------------------------
    // 2. ANTI-DEBUG  (IsDebuggerPresent + NtGlobalFlag + heap flags)
    // -----------------------------------------------------------------------
    inline bool IsDebuggerAttached()
    {
        // Method 1: standard API
        if (IsDebuggerPresent()) return true;

        // Method 2: PEB.NtGlobalFlag at offset 0xBC (x64) — set to 0x70 when debugged
        auto* pPEB = reinterpret_cast<uint8_t*>(__readgsqword(0x60));
        uint32_t ntGlobalFlag = *reinterpret_cast<uint32_t*>(pPEB + 0xBC);
        if (ntGlobalFlag & 0x70) return true;

        // Method 3: Heap ForceFlags — always 0 on normal heaps, non-zero when debugged
        // NOTE: flags & 0x2 (HEAP_GROWABLE) is set on ALL normal heaps — do NOT check it
#if defined(_WIN64)
        auto heapBase = *reinterpret_cast<uint64_t*>(pPEB + 0x30);
        if (heapBase) {
            uint32_t forceFlag = *reinterpret_cast<uint32_t*>(heapBase + 0x74);
            if (forceFlag != 0) return true;
        }
#endif

        // NOTE: Timing check removed — QPC jitter from CPU scheduling
        // regularly exceeds any tight threshold on real machines (false positives)

        return false;
    }

    // -----------------------------------------------------------------------
    // 3. ANTI-VM / SANDBOX DETECTION
    //    Checks for known VM artifacts, short uptime, and single-core systems.
    // -----------------------------------------------------------------------
    inline bool IsVMOrSandbox()
    {
        // Check CPU hypervisor bit
        int cpuid[4] = {};
        __cpuid(cpuid, 1);
        if (cpuid[2] & (1 << 31)) return true; // hypervisor bit set

        // Single logical core = typical sandbox
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        if (si.dwNumberOfProcessors <= 1) return true;

        // NOTE: Uptime check removed — legitimate users reboot machines.
        // Short uptime is not a reliable sandbox indicator on real hardware.

        // Check for known VM process names
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe{ sizeof(pe) };
            if (Process32FirstW(snap, &pe)) {
                do {
                    std::wstring name = pe.szExeFile;
                    // Convert to lowercase
                    for (auto& c : name) c = towlower(c);
                    if (name == xorstr_(L"vboxservice.exe")  ||
                        name == xorstr_(L"vmtoolsd.exe")     ||
                        name == xorstr_(L"vmwaretray.exe")   ||
                        name == xorstr_(L"vmwareuser.exe")   ||
                        name == xorstr_(L"vmsrvc.exe")       ||
                        name == xorstr_(L"sandboxierpcss.exe") ||
                        name == xorstr_(L"sbiesvc.exe")      ||
                        name == xorstr_(L"sbiectrl.exe")     ||
                        name == xorstr_(L"wireshark.exe")    ||
                        name == xorstr_(L"x32dbg.exe")       ||
                        name == xorstr_(L"x64dbg.exe")       ||
                        name == xorstr_(L"ollydbg.exe")      ||
                        name == xorstr_(L"idaq.exe")         ||
                        name == xorstr_(L"idaq64.exe")       ||
                        name == xorstr_(L"processhacker.exe"))
                    {
                        CloseHandle(snap);
                        return true;
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
        }

        return false;
    }

    // -----------------------------------------------------------------------
    // 4. PREFETCH CLEANUP
    //    Deletes our own .pf file from C:\Windows\Prefetch on exit.
    // -----------------------------------------------------------------------
    inline void CleanPrefetch()
    {
        wchar_t exePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);

        // Extract just the filename (no path, no extension) and uppercase it
        std::wstring full(exePath);
        auto slash = full.rfind(L'\\');
        std::wstring fname = (slash != std::wstring::npos)
            ? full.substr(slash + 1) : full;
        auto dot = fname.rfind(L'.');
        if (dot != std::wstring::npos) fname = fname.substr(0, dot);
        for (auto& c : fname) c = towupper(c);

        // Enumerate C:\Windows\Prefetch for matching .pf files
        wchar_t sysRoot[MAX_PATH]{};
        GetWindowsDirectoryW(sysRoot, MAX_PATH);
        std::wstring pfDir = std::wstring(sysRoot) + xorstr_(L"\\Prefetch\\");
        std::wstring pattern = pfDir + fname + xorstr_(L"-*.pf");

        WIN32_FIND_DATAW fd{};
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::wstring full_pf = pfDir + fd.cFileName;
                DeleteFileW(full_pf.c_str());
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }

    // -----------------------------------------------------------------------
    // 5. PEB PROCESS NAME SPOOF
    //    Overwrites the PEB's ImagePathName and ProcessParameters
    //    so tools reading the PEB see a benign name like "explorer.exe"
    // -----------------------------------------------------------------------
    inline void SpoofPEBProcessName()
    {
        // Access PEB via raw offsets — avoids winternl.h's incomplete PEB stub
        // x64: GS:[0x60] -> PEB, PEB+0x20 -> ProcessParameters ptr
        auto* pPEB = reinterpret_cast<uint8_t*>(__readgsqword(0x60));
        if (!pPEB) return;

        auto* procParams = *reinterpret_cast<uint8_t**>(pPEB + 0x20);
        if (!procParams) return;

        // UNICODE_STRING ImagePathName is at offset 0x60 in RTL_USER_PROCESS_PARAMETERS (x64)
        // struct UNICODE_STRING { USHORT Length; USHORT MaximumLength; PWSTR Buffer; }
        static wchar_t fakeName[] = L"explorer.exe";
        static uint16_t fakeLen    = static_cast<uint16_t>((wcslen(fakeName)) * sizeof(wchar_t));
        static uint16_t fakeMaxLen = fakeLen + sizeof(wchar_t);

        auto* imagePathName = procParams + 0x60; // ImagePathName UNICODE_STRING
        DWORD oldProt = 0;
        VirtualProtect(imagePathName, 16, PAGE_READWRITE, &oldProt);
        *reinterpret_cast<uint16_t*>(imagePathName + 0) = fakeLen;
        *reinterpret_cast<uint16_t*>(imagePathName + 2) = fakeMaxLen;
        *reinterpret_cast<wchar_t**>(imagePathName + 8) = fakeName;
        VirtualProtect(imagePathName, 16, oldProt, &oldProt);
    }

    // -----------------------------------------------------------------------
    // RunAll — call once at startup
    // Returns false if analysis environment detected (caller should exit)
    // -----------------------------------------------------------------------
    inline bool RunAll()
    {
        if (IsDebuggerPresent()) return false;

        // PEB NtGlobalFlag — set to 0x70 only under a real debugger
        auto* pPEB = reinterpret_cast<uint8_t*>(__readgsqword(0x60));
        uint32_t ntGlobalFlag = *reinterpret_cast<uint32_t*>(pPEB + 0xBC);
        if (ntGlobalFlag & 0x70) return false;

        // Heap ForceFlags — always 0 on normal heaps
#if defined(_WIN64)
        auto heapBase = *reinterpret_cast<uint64_t*>(pPEB + 0x30);
        if (heapBase) {
            uint32_t forceFlag = *reinterpret_cast<uint32_t*>(heapBase + 0x74);
            if (forceFlag != 0) return false;
        }
#endif

        // NOTE: CPUID hypervisor bit removed — Windows 11 VBS/Hyper-V sets it
        // on all real hardware machines, causing false positives for all users.

        // Single logical core = sandbox (real machines always have 2+)
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        if (si.dwNumberOfProcessors <= 1) return false;

        // Known debugger / analysis tool process names
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe{ sizeof(pe) };
            if (Process32FirstW(snap, &pe)) {
                do {
                    std::wstring name = pe.szExeFile;
                    for (auto& c : name) c = towlower(c);
                    if (name == xorstr_(L"vboxservice.exe")    ||
                        name == xorstr_(L"vmtoolsd.exe")       ||
                        name == xorstr_(L"vmwaretray.exe")     ||
                        name == xorstr_(L"vmwareuser.exe")     ||
                        name == xorstr_(L"sandboxierpcss.exe") ||
                        name == xorstr_(L"sbiesvc.exe")        ||
                        name == xorstr_(L"x32dbg.exe")         ||
                        name == xorstr_(L"x64dbg.exe")         ||
                        name == xorstr_(L"ollydbg.exe")        ||
                        name == xorstr_(L"idaq.exe")           ||
                        name == xorstr_(L"idaq64.exe")         ||
                        name == xorstr_(L"processhacker.exe"))
                    {
                        CloseHandle(snap);
                        return false;
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
        }

        SpoofPEBProcessName();
        // NOTE: WipePEHeader() removed — DX11/runtime still reads PE headers
        // after load (resource lookup, SEH unwind tables) causing a crash at auth init.
        atexit([]() { CleanPrefetch(); });
        return true;
    }

} // namespace Security
