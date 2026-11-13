#include "Interface.hpp"
#include <Main/Options.hpp>
#include "Main/xorstring.h"
#include <thread>
#include "kaanlar.h"
#include <chrono>
#include <string>
#include <sstream>
#include <wincrypt.h>
#include <filesystem>
#include <vector>
#include "Utils/Dependencies/ImGui/framework/headers/includes.h"
#include "Utils/Dependencies/ImGui/framework/headers/functions.h"
#include "Utils/Dependencies/ImGui/framework/data/fonts.h"
#include "Utils/Dependencies/ImGui/framework/data/images.h"


M_base::Interface interfaceObject;


#pragma comment(lib, "crypt32.lib")


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool LeftMouseDown = false;
ImVec2 LastMousePos = ImVec2(0, 0);
ImRect WindowRect = ImRect();

using namespace ImGui;

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD picker_flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_DisplayHex;

namespace M_base {
    void Interface::Initialize(HWND Window, HWND TargetWindow, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext) {
        hWindow = Window;
        hTargetWindow = TargetWindow;
        IDevice = Device;
        menuInstance = nullptr;

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hWindow);
        ImGui_ImplDX11_Init(Device, DeviceContext);
        
        font->update();
    }

    void Interface::RenderGui() {
        if (!bIsMenuOpen)
            return;
            gui->render();

    }

    void Interface::RenderWatermark() {
    }

    void Interface::process_keybinds() {
        cfg->process_keybinds();
    }

    void Interface::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                ResizeWidht = (UINT)LOWORD(lParam);
                ResizeHeight = (UINT)HIWORD(lParam);
            }
            break;
        case WM_INPUT:
            if (bIsMenuOpen && ::GetForegroundWindow() != hWnd)
            {
                RAWINPUT raw{};
                UINT dwSize = sizeof(RAWINPUT);
                if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) != (UINT)-1)
                {
                    if (raw.header.dwType == RIM_TYPEMOUSE && (raw.data.mouse.usButtonFlags & RI_MOUSE_WHEEL))
                    {
                        short wheelDelta = static_cast<short>(raw.data.mouse.usButtonData);
                        float wheel = static_cast<float>(wheelDelta) / 120.0f;

                        POINT clientPt;
                        if (::GetCursorPos(&clientPt))
                        {
                            ::ScreenToClient(hWnd, &clientPt);
                            bool isOverMenu = false;
                            ImGuiContext* g = ImGui::GetCurrentContext();
                            if (g != nullptr)
                            {
                                ImGuiWindow* menuWindow = ImGui::FindWindowByName("menu");
                                if (menuWindow)
                                {
                                    ImRect menuRect(menuWindow->Pos, menuWindow->Pos + menuWindow->Size);
                                    if (menuRect.Contains(ImVec2(static_cast<float>(clientPt.x), static_cast<float>(clientPt.y))))
                                        isOverMenu = true;
                                }
                                if (!isOverMenu && g->HoveredWindow != nullptr)
                                {
                                    isOverMenu = true;
                                }
                            }
                            if (!isOverMenu && gui && gui->is_popup_open())
                            {
                                isOverMenu = true;
                            }

                            if (isOverMenu)
                            {
                                float cur = m_MouseWheelDelta.load(std::memory_order_relaxed);
                                while (!m_MouseWheelDelta.compare_exchange_weak(cur, cur + wheel, std::memory_order_relaxed)) {}
                            }
                        }
                    }
                }
            }
            break;
        }
        if (bIsMenuOpen) {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        }
    }

    void Interface::HandleWindowDrag() {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!LeftMouseDown)
                LastMousePos = ImGui::GetMousePos();
            LeftMouseDown = true;
        }
        else {
            LeftMouseDown = false;
        }
        ImVec2 CurrentMousePos = ImGui::GetMousePos();
        if (CurrentMousePos.x != LastMousePos.x || CurrentMousePos.y != LastMousePos.y) {
            if (LeftMouseDown) {
                if (WindowRect.Contains(CurrentMousePos)) {
                    ImVec2 Delta = CurrentMousePos - LastMousePos;
                    ImVec2 Min = ImVec2(0, 0);
                    ImVec2 Max = ImGui::GetIO().DisplaySize - WindowRect.GetSize();
                    ImVec2 Pos = ImClamp(WindowRect.Min + Delta, Min, Max);
                    WindowRect.Min = Pos;
                    LastMousePos = CurrentMousePos;
                }
            }
        }
        ImGui::SetNextWindowPos(WindowRect.Min);
        ImGui::SetNextWindowSize(WindowRect.GetSize());
    }

    void Interface::HandleMenuKey() {
        static bool MenuKeyDown = false;
        if (Cheat::g_IsBindingKey) {
            MenuKeyDown = true;
            return;
        }
        if (g_Options.General.MenuKey != 0 && (GetAsyncKeyState(g_Options.General.MenuKey) & 0x8000)) {
            if (!MenuKeyDown) {
                MenuKeyDown = true;
                bIsMenuOpen = !bIsMenuOpen;
            }
        }
        else {
            MenuKeyDown = false;
        }

        // Stream Proof dynamic update: exclude overlay from Discord, OBS, and Nvidia ShadowPlay captures
        static int lastStreamProof = -1;
        int currentStreamProof = g_Options.General.StreamProof ? 1 : 0;
        if (currentStreamProof != lastStreamProof) {
            lastStreamProof = currentStreamProof;
            if (hWindow) {
                DWORD affinity = g_Options.General.StreamProof ? 0x00000011 : 0x00000000;
                if (!SetWindowDisplayAffinity(hWindow, affinity)) {
                    if (g_Options.General.StreamProof) {
                        SetWindowDisplayAffinity(hWindow, 0x00000001);
                    }
                }
            }
        }

        static bool lastMenuOpen = false;
        static bool lastBlockInput = false;
        static bool lastNeedsInput = false;
        static bool firstRun = true;

        bool needsInput = gui->is_popup_open();
        bool mouseHeld = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if ((firstRun || bIsMenuOpen != lastMenuOpen || (bIsMenuOpen && g_Options.General.BlockInput != lastBlockInput) || (bIsMenuOpen && needsInput != lastNeedsInput)) && (!mouseHeld || firstRun || bIsMenuOpen != lastMenuOpen)) {
            firstRun = false;
            lastMenuOpen = bIsMenuOpen;
            lastBlockInput = g_Options.General.BlockInput;
            lastNeedsInput = needsInput;

            if (bIsMenuOpen) {
                // Register Raw Input sink so mouse wheel is received without blocking the game
                RAWINPUTDEVICE rid;
                rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
                rid.usUsage     = 0x02; // HID_USAGE_GENERIC_MOUSE
                rid.dwFlags     = RIDEV_INPUTSINK;
                rid.hwndTarget  = hWindow;
                RegisterRawInputDevices(&rid, 1, sizeof(rid));

                if (g_Options.General.BlockInput || needsInput) {
                    // Block Input enabled or text input needed: mouse & keyboard input affects overlay, background apps receive no input
                    SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED);
                    SetWindowPos(hWindow, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                    SetForegroundWindow(hWindow);
                    SetFocus(hWindow);
                }
                else {
                    // Block Input disabled and no text input needed: window is transparent to clicks/moves, affects background apps too
                    SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED);
                    SetWindowPos(hWindow, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                    if (hTargetWindow) SetForegroundWindow(hTargetWindow);
                }
            }
            else {
                // Menu closed: unregister raw input sink so zero raw mouse messages are dispatched
                RAWINPUTDEVICE rid;
                rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
                rid.usUsage     = 0x02; // HID_USAGE_GENERIC_MOUSE
                rid.dwFlags     = RIDEV_REMOVE;
                rid.hwndTarget  = NULL;
                RegisterRawInputDevices(&rid, 1, sizeof(rid));

                // Menu closed: overlay passes clicks through
                SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED);
                SetWindowPos(hWindow, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                if (hTargetWindow) SetForegroundWindow(hTargetWindow);
            }
        }
    }

    void Interface::ShutDown() {
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
        rid.usUsage     = 0x02; // HID_USAGE_GENERIC_MOUSE
        rid.dwFlags     = RIDEV_REMOVE;
        rid.hwndTarget  = NULL;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    
}
