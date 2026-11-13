#include "Memory.hpp"
#include "Syscall.hpp"
#include <Utils/Render/kaanlar.h>
#include <Utils/FrameWork.hpp>
#include <winternl.h>
#include <psapi.h>
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "psapi.lib")

struct TGetWindowHandleData
{
	DWORD Pid;
	std::wstring WindowName;
	HWND hWnd;
};

namespace M_base
{
	namespace mem
	{
		HANDLE AttachedProcessHandle;
		DWORD AttachedProcessPid;
	}
}

BOOL CALLBACK EnumWindowsCallback(HWND Handle, LPARAM lParam)
{
	TGetWindowHandleData& Data = *(TGetWindowHandleData*)lParam;

	// Searching For Name
	if (Data.Pid == 0)
	{
		int Length = SafeCall(GetWindowTextLength)(Handle);
		if (Length == 0)
			return true;

		std::wstring Buffer(Length + 1, L'\0');

		SafeCall(GetWindowText)(Handle, &Buffer[0], Length + 1);

		if (Data.WindowName != Buffer)
			return true;

		Data.hWnd = Handle;
		return false;
	}
	else // Searching for Pid
	{
		DWORD Pid;
		SafeCall(GetWindowThreadProcessId)(Handle, &Pid);

		if (Data.Pid != Pid)
			return true;

		if (!IsWindowVisible(Handle) || GetWindow(Handle, GW_OWNER) != nullptr)
			return true;

		RECT rc{};
		if (!GetClientRect(Handle, &rc) || (rc.right - rc.left) < 100 || (rc.bottom - rc.top) < 100)
			return true;

		Data.hWnd = Handle;
		return false;
	}

	return true;
}

namespace M_base
{
	HWND mem::GetWindowHandleByPID(int Pid)
	{
		TGetWindowHandleData HandleData;
		HandleData.Pid = Pid;
		HandleData.WindowName = xorstr_(L"");
		HandleData.hWnd = NULL;

		SafeCall(EnumWindows)(EnumWindowsCallback, (LPARAM)&HandleData);
		return HandleData.hWnd;
	}

	HWND mem::GetWindowHandleByName(std::wstring WindowName)
	{
		TGetWindowHandleData HandleData;
		HandleData.Pid = 0;
		HandleData.WindowName = WindowName;
		HandleData.hWnd = NULL;

		SafeCall(EnumWindows)(EnumWindowsCallback, (LPARAM)&HandleData);
		return HandleData.hWnd;
	}

	DWORD mem::GetProcessPidByName(std::wstring ProcessName)
	{
		HANDLE hSnapshot = SafeCall(CreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, 0);
		if (!hSnapshot || hSnapshot == INVALID_HANDLE_VALUE || hSnapshot == ((HANDLE)(LONG_PTR)ERROR_BAD_LENGTH))
		{

			return 0;
		}

		DWORD Pid;
		PROCESSENTRY32 ProcessEntry;
		ProcessEntry.dwSize = sizeof(ProcessEntry);
		if (SafeCall(Process32First)(hSnapshot, &ProcessEntry))
		{
			while (_wcsicmp(ProcessEntry.szExeFile, ProcessName.c_str()))
			{
				if (!SafeCall(Process32Next)(hSnapshot, &ProcessEntry))
				{
					SafeCall(CloseHandle)(hSnapshot);
					return 0;
				}
			}

			Pid = ProcessEntry.th32ProcessID;
		}
		else
		{
			SafeCall(CloseHandle)(hSnapshot);
			return 0;
		}

		SafeCall(CloseHandle)(hSnapshot);
		return Pid;
	}

	uint64_t mem::GetModuleBaseByName(DWORD Pid, std::wstring ModuleName)
	{
		// --- Method 1: EnumProcessModules (not hooked by EAC) ---
		HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Pid);
		if (hProc)
		{
			HMODULE mods[1024];
			DWORD needed = 0;
			if (EnumProcessModulesEx(hProc, mods, sizeof(mods), &needed, LIST_MODULES_ALL))
			{
				DWORD count = needed / sizeof(HMODULE);
				for (DWORD i = 0; i < count; i++)
				{
					wchar_t name[MAX_PATH] = {};
					if (GetModuleFileNameExW(hProc, mods[i], name, MAX_PATH))
					{
						// compare just the filename portion
						std::wstring fullPath(name);
						size_t pos = fullPath.rfind(L'\\');
						std::wstring baseName = (pos != std::wstring::npos)
							? fullPath.substr(pos + 1) : fullPath;

						if (_wcsicmp(baseName.c_str(), ModuleName.c_str()) == 0)
						{
							uint64_t base = (uint64_t)(ULONG_PTR)mods[i];
							CloseHandle(hProc);
							return base;
						}
					}
				}
			}
			CloseHandle(hProc);
		}

		// --- Method 2: TH32CS_SNAPMODULE fallback (may fail under EAC) ---
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, Pid);
		if (hSnapshot && hSnapshot != INVALID_HANDLE_VALUE)
		{
			MODULEENTRY32W ModuleEntry;
			ModuleEntry.dwSize = sizeof(ModuleEntry);
			if (Module32FirstW(hSnapshot, &ModuleEntry))
			{
				do {
					if (_wcsicmp(ModuleEntry.szModule, ModuleName.c_str()) == 0)
					{
						uint64_t base = (uint64_t)(ULONG_PTR)ModuleEntry.modBaseAddr;
						CloseHandle(hSnapshot);
						return base;
					}
				} while (Module32NextW(hSnapshot, &ModuleEntry));
			}
			CloseHandle(hSnapshot);
		}

		return 0;
	}

	void mem::AttachProces(DWORD Pid)
	{
		AttachedProcessPid = Pid;

		// Initialize the direct-syscall bypass BEFORE opening handle
		// (SSNs are read from ntdll before EAC patches them at process start,
		//  but we still need a valid PROCESS_VM_READ handle)
		SysCall::Init();

		// Try PROCESS_ALL_ACCESS first; if EAC denies it fall back to VM_READ only
		AttachedProcessHandle = SafeCall(OpenProcess)(PROCESS_ALL_ACCESS, FALSE, Pid);
		if (!AttachedProcessHandle)
		{
			AttachedProcessHandle = SafeCall(OpenProcess)(
				PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, Pid);
		}
	}

	void mem::DetachProcess()
	{
		SafeCall(CloseHandle)(AttachedProcessHandle);
		AttachedProcessHandle = nullptr;
		AttachedProcessPid = 0x0;
	}

	void mem::ReadProcessMemoryImpl(uint64_t ReadAddress, LPVOID Read, SIZE_T Size)
	{
		if (!AttachedProcessHandle || !AttachedProcessPid)
			return;

		// Primary path: direct NT syscall (bypasses EAC IAT hook on ReadProcessMemory)
		if (SysCall::g_bInitialized)
		{
			SysCall::NtReadVMem(AttachedProcessHandle, ReadAddress, Read, Size);
			return;
		}

		// Fallback: standard Win32 API
		SafeCall(ReadProcessMemory)(AttachedProcessHandle, (LPVOID)ReadAddress, Read, Size, nullptr);
	}

	bool mem::WriteProcessMemoryImpl(uint64_t WriteAddress, LPVOID Value, SIZE_T Size)
	{
		if (!AttachedProcessHandle || !AttachedProcessPid)
			return false;

		// Primary path: direct NT syscall
		if (SysCall::g_bInitialized)
		{
			NTSTATUS st = SysCall::NtWriteVMem(AttachedProcessHandle, WriteAddress, Value, Size);
			return (st == 0);
		}

		// Fallback
		return SafeCall(WriteProcessMemory)(AttachedProcessHandle, (LPVOID)WriteAddress, Value, Size, nullptr) != 0;
	}

	std::string mem::ReadProcessMemoryString(uint64_t ReadAddress, SIZE_T MaxSize /*= 256*/)
	{
		std::string result;
		result.reserve(MaxSize);

		for (SIZE_T i = 0; i < MaxSize; i++)
		{
			char c = 0;
			ReadProcessMemoryImpl(ReadAddress + i, &c, sizeof(char));

			if (c == '\0')
				break; // null terminator, end of string

			result.push_back(c);
		}

		return result;
	}

}