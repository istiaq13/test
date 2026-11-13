#include <Windows.h>
#include <TlHelp32.h>
#include <thread>
#include <iostream>
#include <entryPoint.hpp>
#include <Main/Cheat.hpp>
#include <Main/Auth/AuthWindow.hpp>
#include <Main/Auth/AuthClient.hpp>
#include <Utils/Security/Evasion.hpp>
#include <Utils/Render/kaanlar.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

static bool IsProcessElevated()
{
    BOOL isElevated = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation;
        DWORD dwSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
            isElevated = elevation.TokenIsElevated != 0;
        CloseHandle(hToken);
    }
    return isElevated != FALSE;
}

static bool RelaunchElevated()
{
    wchar_t szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH))
    {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb       = L"runas";
        sei.lpFile       = szPath;
        sei.lpParameters = GetCommandLineW();
        sei.nShow        = SW_NORMAL;
        if (ShellExecuteExW(&sei)) return true;
    }
    return false;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    if (!IsProcessElevated())
    {
        if (RelaunchElevated()) return 0;
    }

    // Cleanup updater leftovers
    wchar_t szModPath[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, szModPath, MAX_PATH))
    {
        std::wstring sOld = std::wstring(szModPath) + L".old";
        std::wstring sNew = std::wstring(szModPath) + L".new";
        SetFileAttributesW(sOld.c_str(), FILE_ATTRIBUTE_NORMAL); DeleteFileW(sOld.c_str());
        SetFileAttributesW(sNew.c_str(), FILE_ATTRIBUTE_NORMAL); DeleteFileW(sNew.c_str());
    }

    atexit([]() { Cheat::Auth::ConfigCache::Wipe(); });

    if (!Security::RunAll())
    {
        Cheat::Auth::ConfigCache::Wipe();
        return 0;
    }

    M_base::Misc::OptimizeThreadForGameSync();
#ifdef _DEBUG
    M_base::Misc::InitializeConsole();
    std::cout << "[RobRun] Initializing..." << std::endl;
#endif

    bool authenticated = Cheat::Auth::AuthWindow::Run();
    if (!authenticated)
    {
#ifdef _DEBUG
        M_base::Misc::ShutDownConsole();
#endif
        Cheat::Auth::ConfigCache::Wipe();
        return 0;
    }

#ifdef _DEBUG
    std::cout << "[RobRun] Authenticated: "
              << Cheat::Auth::AuthWindow::GetCurrentUsername() << std::endl;
#endif

    std::thread cheatThread(Cheat::C_Start);
    cheatThread.detach();

    while (true)
    {
        if (g_Options.General.ShutDown)
        {
            Cheat::C_Stop();
            Cheat::Auth::ConfigCache::Wipe();
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

