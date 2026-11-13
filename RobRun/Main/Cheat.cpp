#include "Cheat.hpp"
#include "Features/esp.hpp"
#include "Utils/Dependencies/ImGui/framework/headers/includes.h"
#include "Utils/Dependencies/ImGui/framework/headers/config.h"
#include "Utils/Dependencies/ImGui/framework/headers/widgets.h"
#include "Utils/Dependencies/ImGui/framework/headers/functions.h"
#include "Options.hpp"
#include "Auth/AuthClient.hpp"
#include "Auth/VoiceRecorder.hpp"
#include "Auth/RealtimeClient.hpp"
#include <atomic>
#include <chrono>
#include <thread>

bool FontUpdated = false;

namespace Cheat
{
    void StartThreads()
    {
        Sleep(2000);
        std::thread([]() { C_RobRun.UpdateEntityList(); }).detach();
        std::thread([]() { Cheat::StartConfiguration(); }).detach();
        Cheat::Voice::VoiceRecorder::InitializeAutoVoice();
        Cheat::Auth::RealtimeClient::Start();
    }

    static void ProcessImGuiKeyboardInput(ImGuiIO& io)
    {
        HWND hOverlay = M_base::Overlay::GetOverlayWindow();
        bool isOverlayForeground = (hOverlay != nullptr && ::GetForegroundWindow() == hOverlay);
        if (isOverlayForeground) return;

        static bool s_PrevKeyDown[256] = { false };
        static float s_KeyRepeatTimer[256] = { 0.f };

        BYTE kState[256];
        if (!GetKeyboardState(kState)) memset(kState, 0, sizeof(kState));

        bool shiftDown = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
        bool ctrlDown  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool altDown   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;

        kState[VK_SHIFT]   = shiftDown ? 0x80 : 0;
        kState[VK_CONTROL] = ctrlDown  ? 0x80 : 0;
        kState[VK_MENU]    = altDown   ? 0x80 : 0;
        kState[VK_CAPITAL] = (GetKeyState(VK_CAPITAL) & 1) ? 0x01 : 0;

        io.AddKeyEvent(ImGuiMod_Shift, shiftDown);
        io.AddKeyEvent(ImGuiMod_Ctrl,  ctrlDown);
        io.AddKeyEvent(ImGuiMod_Alt,   altDown);

        float dt = io.DeltaTime > 0.f ? io.DeltaTime : 0.016f;

        for (int vk = 8; vk <= 255; vk++)
        {
            if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON ||
                vk == VK_XBUTTON1 || vk == VK_XBUTTON2) continue;
            if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) continue;

            bool isDown  = (GetAsyncKeyState(vk) & 0x8000) != 0;
            bool wasDown = s_PrevKeyDown[vk];

            ImGuiKey imKey = ImGuiKey_None;
            if (vk >= '0' && vk <= '9') imKey = (ImGuiKey)(ImGuiKey_0 + (vk - '0'));
            else if (vk >= 'A' && vk <= 'Z') imKey = (ImGuiKey)(ImGuiKey_A + (vk - 'A'));
            else if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
                imKey = (ImGuiKey)(ImGuiKey_Keypad0 + (vk - VK_NUMPAD0));
            else {
                switch (vk) {
                case VK_BACK:       imKey = ImGuiKey_Backspace;   break;
                case VK_TAB:        imKey = ImGuiKey_Tab;         break;
                case VK_RETURN:     imKey = ImGuiKey_Enter;       break;
                case VK_ESCAPE:     imKey = ImGuiKey_Escape;      break;
                case VK_SPACE:      imKey = ImGuiKey_Space;       break;
                case VK_LEFT:       imKey = ImGuiKey_LeftArrow;   break;
                case VK_RIGHT:      imKey = ImGuiKey_RightArrow;  break;
                case VK_UP:         imKey = ImGuiKey_UpArrow;     break;
                case VK_DOWN:       imKey = ImGuiKey_DownArrow;   break;
                case VK_DELETE:     imKey = ImGuiKey_Delete;      break;
                case VK_HOME:       imKey = ImGuiKey_Home;        break;
                case VK_END:        imKey = ImGuiKey_End;         break;
                case VK_OEM_1:      imKey = ImGuiKey_Semicolon;   break;
                case VK_OEM_PLUS:   imKey = ImGuiKey_Equal;       break;
                case VK_OEM_COMMA:  imKey = ImGuiKey_Comma;       break;
                case VK_OEM_MINUS:  imKey = ImGuiKey_Minus;       break;
                case VK_OEM_PERIOD: imKey = ImGuiKey_Period;      break;
                case VK_OEM_2:      imKey = ImGuiKey_Slash;       break;
                case VK_OEM_3:      imKey = ImGuiKey_GraveAccent; break;
                }
            }

            if (isDown != wasDown)
            {
                s_PrevKeyDown[vk] = isDown;
                s_KeyRepeatTimer[vk] = 0.f;
                if (!isOverlayForeground && imKey != ImGuiKey_None)
                    io.AddKeyEvent(imKey, isDown);
                if (isDown && !isOverlayForeground)
                {
                    WCHAR wch[4] = { 0 };
                    UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
                    int count = ToUnicode(vk, sc, kState, wch, 4, 0);
                    for (int i = 0; i < count; i++)
                        if (wch[i] >= 32 && wch[i] != 127)
                            io.AddInputCharacter(wch[i]);
                }
            }
            else if (isDown)
            {
                if (!isOverlayForeground)
                {
                    s_KeyRepeatTimer[vk] += dt;
                    if (s_KeyRepeatTimer[vk] >= 0.40f)
                    {
                        s_KeyRepeatTimer[vk] -= 0.05f;
                        if (imKey != ImGuiKey_None)
                        { io.AddKeyEvent(imKey, false); io.AddKeyEvent(imKey, true); }
                        WCHAR wch[4] = { 0 };
                        UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
                        int count = ToUnicode(vk, sc, kState, wch, 4, 0);
                        for (int i = 0; i < count; i++)
                            if (wch[i] >= 32 && wch[i] != 127)
                                io.AddInputCharacter(wch[i]);
                    }
                }
            }
            else { s_KeyRepeatTimer[vk] = 0.f; }
        }
    }

    void C_Start()
    {
        // Wait until ROB RUN.exe is found and initialized
        while (!C_RobRun.IsInitialized())
        {
            C_RobRun.Initialize();
            if (!C_RobRun.IsInitialized())
                std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        M_base::Overlay::Setup(C_RobRun.GetPid());
        M_base::Overlay::Initialize();

        if (M_base::Overlay::IsInitialized())
        {
            M_base::Misc::OptimizeThreadForGameSync();
            M_base::Interface Interface(
                M_base::Overlay::GetOverlayWindow(),
                M_base::Overlay::GetTargetWindow(),
                M_base::Overlay::dxGetDevice(),
                M_base::Overlay::dxGetDeviceContext());

            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = NULL;

            M_base::Overlay::SetupWindowProcHook(
                std::bind(&M_base::Interface::WindowProc, &Interface,
                          std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4));

            std::thread([&Interface]() { Interface.process_keybinds(); }).detach();

            // Game-exit watchdog
            std::thread([pid = C_RobRun.m_Pid]()
            {
                HANDLE hGame = OpenProcess(SYNCHRONIZE, FALSE, pid);
                if (hGame)
                {
                    WaitForSingleObject(hGame, INFINITE);
                    CloseHandle(hGame);
                }
                else
                {
                    while (GetProcessVersion(pid) != 0)
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                }
                Cheat::Voice::VoiceRecorder::ShutdownAutoVoice();
                Cheat::Auth::RealtimeClient::Stop();
                Cheat::Auth::ConfigCache::Wipe();
                Cheat::Auth::AuthClient::StopTelemetry();
                ExitProcess(0);
            }).detach();

            cfg->init_config();
            std::thread(StartThreads).detach();

            MSG Message;
            ZeroMemory(&Message, sizeof(Message));
            while (Message.message != WM_QUIT)
            {
                auto frameStart = std::chrono::high_resolution_clock::now();

                while (::PeekMessage(&Message, nullptr, 0U, 0U, PM_REMOVE))
                {
                    ::TranslateMessage(&Message);
                    ::DispatchMessage(&Message);
                    if (Message.message == WM_QUIT) break;
                }

                ImGui::GetIO().MouseDrawCursor = Interface.GetMenuOpen();

                if (Interface.ResizeHeight != 0 || Interface.ResizeWidht != 0)
                {
                    M_base::Overlay::dxCleanupRenderTarget();
                    M_base::Overlay::dxGetSwapChain()->ResizeBuffers(0,
                        Interface.ResizeWidht, Interface.ResizeHeight,
                        DXGI_FORMAT_UNKNOWN, 0);
                    Interface.ResizeHeight = Interface.ResizeWidht = 0;
                    M_base::Overlay::dxCreateRenderTarget();
                }

                Interface.HandleMenuKey();

                HWND hTarget = M_base::Overlay::GetTargetWindow();
                if (hTarget && IsIconic(hTarget)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                M_base::Overlay::UpdateWindowPos();

                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();

                if (Interface.GetMenuOpen())
                {
                    POINT mousePos;
                    if (::GetCursorPos(&mousePos))
                    {
                        ::ScreenToClient(M_base::Overlay::GetOverlayWindow(), &mousePos);
                        io.AddMousePosEvent((float)mousePos.x, (float)mousePos.y);
                    }
                    if (!g_Options.General.BlockInput && !gui->is_popup_open())
                    {
                        io.AddMouseButtonEvent(0, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
                        io.AddMouseButtonEvent(1, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
                    }
                    float wheel = Interface.ConsumeMouseWheelDelta();
                    if (wheel != 0.f) io.AddMouseWheelEvent(0.f, wheel);
                    if (gui->is_popup_open()) ProcessImGuiKeyboardInput(io);
                }

                ImGui::NewFrame();

                if (!FontUpdated) { font->update(); FontUpdated = true; }

                // Refresh view matrix once per frame
                C_RobRun.UpdateViewMatrix();

                // Render ESP
                Cheat::ESP::run();

                // GUI
                if (Interface.GetMenuOpen())
                    Interface.RenderGui();

                notify->setup_notify();

                ImGui::EndFrame();
                ImGui::Render();

                M_base::Overlay::dxRefresh();
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                M_base::Overlay::dxGetSwapChain()->Present(1, 0);

                HWND hFg = ::GetForegroundWindow();
                bool isForeground = (hFg == hTarget || hFg == M_base::Overlay::GetOverlayWindow());
                if (!isForeground && !Interface.GetMenuOpen())
                {
                    int64_t targetMicros = 1000000 / 30;
                    auto frameEnd = std::chrono::high_resolution_clock::now();
                    auto elapsed  = std::chrono::duration_cast<std::chrono::microseconds>(
                                        frameEnd - frameStart).count();
                    if (elapsed < targetMicros)
                    {
                        int64_t rem = targetMicros - elapsed;
                        if (rem >= 2000)
                            std::this_thread::sleep_for(std::chrono::milliseconds(rem / 1000));
                    }
                }

                if (g_Options.General.ShutDown)
                {
                    C_Stop();
                    return;
                }
            }
        }
    }

    void C_Stop()
    {
        if (!g_Options.General.ShutDown) return;
        Cheat::Auth::ConfigCache::Wipe();
        Cheat::Auth::AuthClient::StopTelemetry();
        M_base::Overlay::ShutDown();
        M_base::Overlay::dxShutDown();
        M_base::mem::DetachProcess();
    }
}

