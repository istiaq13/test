#pragma once
#include <windows.h>
#include <cstdint>

// ============================================================
//  Direct NT syscall bypass for EasyAntiCheat
//  EAC hooks NtReadVirtualMemory in ntdll's IAT / inline patch.
//  We resolve the syscall number at runtime from the NTDLL on
//  disk (unhooked copy) and invoke it via a manual stub.
// ============================================================

namespace SysCall
{
    // NT status type
    using NTSTATUS = LONG;
    static constexpr NTSTATUS STATUS_SUCCESS = 0;

    // ----------------------------------------------------------------
    // Syscall stub — executes "syscall" instruction directly.
    // We build it on the heap at runtime and call it via function ptr.
    // ----------------------------------------------------------------
    inline BYTE*  g_pStub          = nullptr;
    inline DWORD  g_NtReadSSN      = 0;
    inline DWORD  g_NtWriteSSN     = 0;
    inline bool   g_bInitialized   = false;

    // Allocate RWX memory and write a universal syscall stub:
    //   mov r10, rcx
    //   mov eax, <ssn>   -- patched before each call
    //   syscall
    //   ret
    // We use a 2-stub layout (read @ offset 0, write @ offset 16)
    inline bool Init()
    {
        if (g_bInitialized) return true;

        // --- Resolve SSNs from ntdll on disk (before EAC hooks it) ---
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (!hNtdll) return false;

        // Helper: read the SSN (Zw* functions start with: mov eax, <SSN>)
        auto getSSN = [](HMODULE h, const char* name) -> DWORD
        {
            auto fn = reinterpret_cast<BYTE*>(GetProcAddress(h, name));
            if (!fn) return 0;
            // EAC may have patched the first bytes with a jmp. Walk forward
            // up to 32 bytes looking for "B8 xx xx xx xx" (mov eax, imm32)
            for (int i = 0; i < 32; i++)
            {
                if (fn[i] == 0xB8)   // mov eax, imm32
                    return *reinterpret_cast<DWORD*>(fn + i + 1);
            }
            return 0;
        };

        g_NtReadSSN  = getSSN(hNtdll, "NtReadVirtualMemory");
        g_NtWriteSSN = getSSN(hNtdll, "NtWriteVirtualMemory");

        if (!g_NtReadSSN) return false;

        // Allocate executable stub memory
        g_pStub = reinterpret_cast<BYTE*>(
            VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!g_pStub) return false;

        // Write stub bytes (Windows x64 calling convention)
        // Stub layout (20 bytes):
        //   4C 8B D1          mov r10, rcx
        //   B8 XX XX XX XX    mov eax, <SSN>
        //   0F 05             syscall
        //   C3                ret
        // We write two stubs: one for read (offset 0), one for write (offset 24)
        auto writeStub = [](BYTE* p, DWORD ssn)
        {
            p[0] = 0x4C; p[1] = 0x8B; p[2] = 0xD1;              // mov r10, rcx
            p[3] = 0xB8;                                           // mov eax, ...
            *reinterpret_cast<DWORD*>(p + 4) = ssn;               //   <ssn>
            p[8] = 0x0F; p[9] = 0x05;                             // syscall
            p[10] = 0xC3;                                          // ret
        };

        writeStub(g_pStub,      g_NtReadSSN);   // read stub  @ offset 0
        writeStub(g_pStub + 24, g_NtWriteSSN);  // write stub @ offset 24

        g_bInitialized = true;
        return true;
    }

    // ----------------------------------------------------------------
    // NtReadVirtualMemory signature
    // ----------------------------------------------------------------
    using FnNtRVM = NTSTATUS(NTAPI*)(
        HANDLE ProcessHandle,
        PVOID  BaseAddress,
        PVOID  Buffer,
        SIZE_T NumberOfBytesToRead,
        PSIZE_T NumberOfBytesRead);

    // ----------------------------------------------------------------
    // NtWriteVirtualMemory signature  
    // ----------------------------------------------------------------
    using FnNtWVM = NTSTATUS(NTAPI*)(
        HANDLE ProcessHandle,
        PVOID  BaseAddress,
        PVOID  Buffer,
        SIZE_T NumberOfBytesToWrite,
        PSIZE_T NumberOfBytesWritten);

    inline NTSTATUS NtReadVMem(HANDLE hProc, uintptr_t addr, void* buf, SIZE_T size)
    {
        if (!g_bInitialized || !g_pStub) return -1;
        SIZE_T read = 0;
        return reinterpret_cast<FnNtRVM>(g_pStub)(
            hProc, reinterpret_cast<PVOID>(addr), buf, size, &read);
    }

    inline NTSTATUS NtWriteVMem(HANDLE hProc, uintptr_t addr, void* buf, SIZE_T size)
    {
        if (!g_bInitialized || !g_pStub) return -1;
        SIZE_T written = 0;
        return reinterpret_cast<FnNtWVM>(g_pStub + 24)(
            hProc, reinterpret_cast<PVOID>(addr), buf, size, &written);
    }

} // namespace SysCall

