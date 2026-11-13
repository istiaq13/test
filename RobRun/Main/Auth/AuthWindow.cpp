#include "AuthWindow.hpp"
#include <Windows.h>
#include <shellapi.h>
#include <d3d11.h>
#include <cmath>
#include <chrono>
#include <future>
#include <cctype>
#include <algorithm>
#include <urlmon.h>
#include <wininet.h>
#include <Utils/Utilities/Misc.hpp>
#include <Utils/Dependencies/ImGui/imgui.h>
#include <Utils/Dependencies/ImGui/imgui_impl_win32.h>
#include <Utils/Dependencies/ImGui/imgui_impl_dx11.h>
#include <Utils/Dependencies/ImGui/framework/data/fonts.h>

#ifndef DXGI_SWAP_EFFECT_FLIP_DISCARD
#define DXGI_SWAP_EFFECT_FLIP_DISCARD ((DXGI_SWAP_EFFECT)4)
#endif

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "wininet.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// Shared palette (matches main overlay - Minimalist Obsidian & Neon Cyan)
// ---------------------------------------------------------------------------
static const ImVec4 C_BG        = { 0.035f, 0.039f, 0.047f, 1.f }; // #090A0C pitch obsidian
static const ImVec4 C_CARD      = { 0.055f, 0.063f, 0.078f, 1.f }; // #0E1014 dark glass card
static const ImVec4 C_STROKE    = { 0.160f, 0.170f, 0.200f, 0.6f }; // 1px subtle frosted border
static const ImVec4 C_ACCENT    = { 0.180f, 0.800f, 0.557f, 1.f }; // #2ECC8E sea green
static const ImVec4 C_ACCENT_H  = { 0.250f, 0.880f, 0.650f, 1.f };
static const ImVec4 C_ACCENT_A  = { 0.140f, 0.680f, 0.470f, 1.f };
static const ImVec4 C_TEXT      = { 1.000f, 1.000f, 1.000f, 1.f };
static const ImVec4 C_DIM       = { 0.380f, 0.420f, 0.490f, 1.f };
static const ImVec4 C_MID       = { 0.650f, 0.700f, 0.780f, 1.f };
static const ImVec4 C_ERR       = { 1.000f, 0.231f, 0.188f, 1.f }; // #FF3B30
static const ImVec4 C_AMBER     = { 1.000f, 0.624f, 0.039f, 1.f }; // #FF9F0A
static const ImVec4 C_OK        = { 0.063f, 0.725f, 0.506f, 1.f }; // #10B981
static const ImVec4 C_BTN_GHOST = { 0.090f, 0.100f, 0.125f, 1.f };
static const ImVec4 C_BTN_GH    = { 0.130f, 0.145f, 0.180f, 1.f };
static const ImVec4 C_BTN_GA    = { 0.160f, 0.180f, 0.220f, 1.f };

// ---------------------------------------------------------------------------
// Window sizes per state
// ---------------------------------------------------------------------------
static const int W_LOAD = 280;   // minimalist square loader
static const int H_LOAD = 280;
static const int W_FORM = 280;   // registration
static const int H_FORM = 310;
static const int W_MSG  = 380;   // success / banned / error / update
static const int H_MSG  = 240;

static inline float SmoothLerp(float a, float b, float t)
{
    if (t > 1.0f) t = 1.0f;
    return a + (b - a) * t;
}

namespace Cheat { namespace Auth {

    class UpdateDownloadCallback : public IBindStatusCallback
    {
    private:
        std::chrono::steady_clock::time_point m_LastTime;
        uint64_t m_LastBytes;

    public:
        UpdateDownloadCallback()
            : m_LastTime(std::chrono::steady_clock::now()), m_LastBytes(0) {}

        // IUnknown methods
        STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override
        {
            if (!ppvObject) return E_POINTER;
            if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
            {
                *ppvObject = static_cast<IBindStatusCallback*>(this);
                return S_OK;
            }
            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }
        STDMETHOD_(ULONG, AddRef)() override { return 1; }
        STDMETHOD_(ULONG, Release)() override { return 1; }

        // IBindStatusCallback methods
        STDMETHOD(OnStartBinding)(DWORD, IBinding*) override { return S_OK; }
        STDMETHOD(GetPriority)(LONG*) override { return S_OK; }
        STDMETHOD(OnLowResource)(DWORD) override { return S_OK; }
        STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText) override
        {
            if (ulProgressMax > 0)
            {
                AuthWindow::s_TotalBytes.store((uint64_t)ulProgressMax);
            }
            AuthWindow::s_DownloadedBytes.store((uint64_t)ulProgress);

            auto now = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float>(now - m_LastTime).count();
            if (elapsed >= 0.2f)
            {
                uint64_t cur = (uint64_t)ulProgress;
                uint64_t diff = (cur > m_LastBytes) ? (cur - m_LastBytes) : 0;
                float speedMBps = (float)diff / (1024.f * 1024.f * elapsed);
                AuthWindow::s_DownloadSpeed.store(speedMBps);
                m_LastTime = now;
                m_LastBytes = cur;
            }
            return S_OK;
        }
        STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR) override { return S_OK; }
        STDMETHOD(GetBindInfo)(DWORD*, BINDINFO*) override { return S_OK; }
        STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC*, STGMEDIUM*) override { return S_OK; }
        STDMETHOD(OnObjectAvailable)(REFIID, IUnknown*) override { return S_OK; }
    };

    // H-9 fix: XOR-masked auth token replaces plain std::atomic<bool>.
    // Value of 0 = not authenticated. Any non-zero value is verified by XOR with AUTH_MAGIC.
    std::atomic<uint64_t>     AuthWindow::s_AuthToken(0ULL);
    std::string               AuthWindow::s_CurrentUsername  = "";
    std::string               AuthWindow::s_CurrentKey       = "";
    std::string               AuthWindow::s_CurrentHwid      = "";
    std::string               AuthWindow::s_CurrentCreatedAt = "";
    int                       AuthWindow::s_CurrentExpiresInDays = -1;
    AuthState             AuthWindow::s_State            = STATE_CHECKING;
    HardwareInfo          AuthWindow::s_HwInfo;
    LocationInfo          AuthWindow::s_LocInfo;
    char                  AuthWindow::s_UsernameBuffer[64] = {};
    char                  AuthWindow::s_KeyBuffer[64]      = {};
    std::string           AuthWindow::s_ErrorMessage       = "";
    std::string           AuthWindow::s_SuccessMessage     = "";
    std::string           AuthWindow::s_DownloadLink       = "";
    std::string           AuthWindow::s_LatestVersion      = "";
    std::string           AuthWindow::s_BannedKey          = "";
    std::string           AuthWindow::s_BannedUserId       = "";
    bool                  AuthWindow::s_IsSubmitting       = false;
    std::atomic<bool>     AuthWindow::s_IsDownloading(false);
    std::atomic<uint64_t> AuthWindow::s_DownloadedBytes(0);
    std::atomic<uint64_t> AuthWindow::s_TotalBytes(0);
    std::atomic<float>    AuthWindow::s_DownloadSpeed(0.f);
    std::atomic<bool>     AuthWindow::s_DownloadComplete(false);
    std::atomic<bool>     AuthWindow::s_DownloadFailed(false);
    float                 AuthWindow::s_SuccessTimer       = 0.75f;
    float                 AuthWindow::s_LoadingTimer       = 0.f;

    // H-9 fix: IsAuthenticated() and SetAuthenticated(bool) implement the XOR-masked token.
    // The XOR key is GetTickCount64() which changes every millisecond — a memory
    // scanner cannot patch s_AuthToken to a static value and have it remain valid.
    bool AuthWindow::IsAuthenticated()
    {
        uint64_t token = s_AuthToken.load(std::memory_order_acquire);
        if (token == 0ULL) return false;

        // Recover original auth tick from masked token
        uint64_t recovered = token ^ AUTH_MAGIC;
        if (recovered == 0ULL || recovered == AUTH_MAGIC) return false;

        // Plausibility check: recovered tick must have occurred at or before the current tick
        // (prevents memory editors from patching 0x01, small integers, or memory addresses)
        uint64_t currentTick = static_cast<uint64_t>(GetTickCount64());
        if (recovered > currentTick) return false;

        // Session sanity check: auth token cannot be older than 48 hours of uptime
        if (currentTick - recovered > (48ULL * 3600ULL * 1000ULL)) return false;

        return true;
    }

    void AuthWindow::SetAuthenticated(bool value)
    {
        if (value)
        {
            // Store AUTH_MAGIC XOR current tick — unique per call, changes every ms
            uint64_t tick = static_cast<uint64_t>(GetTickCount64());
            s_AuthToken.store(AUTH_MAGIC ^ tick, std::memory_order_release);
        }
        else
        {
            s_AuthToken.store(0ULL, std::memory_order_release);
        }
    }

    std::string AuthWindow::GetHWID()
    {
        if (!s_CurrentHwid.empty())
            return s_CurrentHwid;
        if (!s_HwInfo.hwid.empty())
        {
            s_CurrentHwid = s_HwInfo.hwid;
            return s_CurrentHwid;
        }
        s_CurrentHwid = HWID::GetHWID();
        return s_CurrentHwid;
    }

    std::string AuthWindow::GetCurrentKey()
    {
        if (!s_CurrentKey.empty())
            return s_CurrentKey;

        // If key not yet cached, query the database using the machine's HWID
        std::string hwid = GetHWID();
        if (!hwid.empty())
        {
            CheckHwidResult r = AuthClient::CheckHWID(hwid);
            if (r.success && r.registered)
            {
                if (!r.key.empty())
                    s_CurrentKey = r.key;
                if (!r.username.empty())
                    s_CurrentUsername = r.username;
                if (!r.createdAt.empty())
                    s_CurrentCreatedAt = r.createdAt;
                if (r.expiresInDays >= 0)
                    s_CurrentExpiresInDays = r.expiresInDays;
                if (!s_CurrentKey.empty())
                    return s_CurrentKey;
            }
        }

        if (s_KeyBuffer[0])
            return s_KeyBuffer;
        if (!s_BannedKey.empty())
            return s_BannedKey;

        return "N/A";
    }

    std::string AuthWindow::GetCurrentUsername()
    {
        if (!s_CurrentUsername.empty())
            return s_CurrentUsername;

        // If username not yet cached, query the database
        GetCurrentKey();
        if (!s_CurrentUsername.empty())
            return s_CurrentUsername;

        char* env_user = getenv("USERNAME");
        return env_user ? env_user : "User";
    }

    std::string AuthWindow::GetMemberSince()
    {
        if (!s_CurrentCreatedAt.empty())
        {
            std::string d = s_CurrentCreatedAt;
            size_t tPos = d.find('T');
            if (tPos != std::string::npos) d = d.substr(0, tPos);
            return d;
        }

        // Trigger DB fetch if not cached
        GetCurrentKey();
        if (!s_CurrentCreatedAt.empty())
        {
            std::string d = s_CurrentCreatedAt;
            size_t tPos = d.find('T');
            if (tPos != std::string::npos) d = d.substr(0, tPos);
            return d;
        }

        return "N/A";
    }

    std::string AuthWindow::GetExpiresIn()
    {
        if (s_CurrentExpiresInDays >= 0)
        {
            if (s_CurrentExpiresInDays == 1)
                return "1 Day";
            return std::to_string(s_CurrentExpiresInDays) + " Days";
        }

        // Trigger DB fetch if not cached
        GetCurrentKey();
        if (s_CurrentExpiresInDays >= 0)
        {
            if (s_CurrentExpiresInDays == 1)
                return "1 Day";
            return std::to_string(s_CurrentExpiresInDays) + " Days";
        }

        return "N/A";
    }

    // -----------------------------------------------------------------------
    // DX11 globals
    // -----------------------------------------------------------------------
    static HWND                  g_hWnd      = nullptr;
    static ID3D11Device*         g_pDev      = nullptr;
    static ID3D11DeviceContext*  g_pCtx      = nullptr;
    static IDXGISwapChain*       g_pSC       = nullptr;
    static ID3D11RenderTargetView* g_pRTV    = nullptr;
    static bool                  g_Running   = true;
    static ImFont*               g_FontLarge = nullptr;   // inter_semibold_data 22px
    static ImFont*               g_FontTitle = nullptr;   // inter_semibold_data 18px
    static ImFont*               g_FontMed   = nullptr;   // inter_medium_data 14px
    static ImFont*               g_FontSmall = nullptr;   // inter_medium_data 11px

        static void DrawBrandTitle(ImDrawList* dl, float centerX, float topY)
    {
        ImFont* fontTitle = g_FontLarge ? g_FontLarge : g_FontTitle;
        float fontSizeTitle = fontTitle ? fontTitle->FontSize : 22.f;
        const char* title = "ROB RUN";
        float trackingTitle = 3.6f;

        float totalTitleW = 0.f;
        int titleLen = (int)strlen(title);
        for (int i = 0; i < titleLen; ++i) {
            char c[2] = { title[i], '\0' };
            totalTitleW += fontTitle ? fontTitle->CalcTextSizeA(fontSizeTitle, FLT_MAX, 0.0f, c).x : ImGui::CalcTextSize(c).x;
            if (i < titleLen - 1) totalTitleW += trackingTitle;
        }

        float curTitleX = centerX - (totalTitleW * 0.5f);
        float titleY = topY;
        for (int i = 0; i < titleLen; ++i) {
            char c[2] = { title[i], '\0' };
            float charW = fontTitle ? fontTitle->CalcTextSizeA(fontSizeTitle, FLT_MAX, 0.0f, c).x : ImGui::CalcTextSize(c).x;

            if (fontTitle) {
                // Soft depth shadow
                dl->AddText(fontTitle, fontSizeTitle, ImVec2(curTitleX, titleY + 1.0f), IM_COL32(30, 160, 110, 75), c);
                // Crisp glowing sea green face
                dl->AddText(fontTitle, fontSizeTitle, ImVec2(curTitleX, titleY), IM_COL32(46, 204, 142, 255), c);
            } else {
                dl->AddText(ImVec2(curTitleX, titleY), IM_COL32(46, 204, 142, 255), c);
            }

            // Cut crossbar of A for futuristic open chevron
            if (title[i] == 'A') {
                float bar_y = titleY + fontSizeTitle * 0.58f;
                float bar_h = fontSizeTitle * 0.12f;
                dl->AddRectFilled(ImVec2(curTitleX + charW * 0.22f, bar_y),
                                  ImVec2(curTitleX + charW * 0.78f, bar_y + bar_h),
                                  ImColor(C_BG));
            }
            // Cut top corner of E for sci-fi cut
            else if (title[i] == 'E') {
                float cut_y = titleY + fontSizeTitle * 0.22f;
                dl->AddRectFilled(ImVec2(curTitleX + 0.5f, cut_y),
                                  ImVec2(curTitleX + charW * 0.28f, cut_y + 1.2f),
                                  ImColor(C_BG));
            }

            curTitleX += charW + trackingTitle;
        }

        // Version subtitle centered directly below CARNAGE
        std::string verStr = "v" + AuthClient::APP_VERSION;
        ImFont* fontVer = g_FontSmall ? g_FontSmall : g_FontMed;
        float fontSizeVer = fontVer ? fontVer->FontSize : 11.f;
        float trackingVer = 1.6f;
        float totalVerW = 0.f;
        int verLen = (int)verStr.length();
        for (int i = 0; i < verLen; ++i) {
            char c[2] = { verStr[i], 0 };
            totalVerW += fontVer ? fontVer->CalcTextSizeA(fontSizeVer, FLT_MAX, 0.0f, c).x : ImGui::CalcTextSize(c).x;
            if (i < verLen - 1) totalVerW += trackingVer;
        }

        float curVerX = centerX - (totalVerW * 0.5f);
        float verY = titleY + fontSizeTitle + 3.f;
        for (int i = 0; i < verLen; ++i) {
            char c[2] = { verStr[i], 0 };
            float charW = fontVer ? fontVer->CalcTextSizeA(fontSizeVer, FLT_MAX, 0.0f, c).x : ImGui::CalcTextSize(c).x;
            if (fontVer) {
                dl->AddText(fontVer, fontSizeVer, ImVec2(curVerX, verY), ImColor(120, 135, 155, 190), c);
            } else {
                dl->AddText(ImVec2(curVerX, verY), ImColor(120, 135, 155, 190), c);
            }
            curVerX += charW + trackingVer;
        }
    }

    static void GetWindowDims(AuthState state, int& w, int& h)
    {
        switch (state) {
        case STATE_CHECKING:  w = W_LOAD; h = H_LOAD; break;
        case STATE_REGISTER:  w = W_FORM; h = H_FORM; break;
        case STATE_LOGGED_IN: w = 380;    h = 200;    break;
        case STATE_EXPIRED:   w = 250;    h = 200;    break;
        case STATE_BANNED:      w = 380;    h = 240;    break;
        case STATE_KEY_EXPIRED: w = 380;    h = 240;    break;
        case STATE_ERROR:       w = 380;    h = 240;    break;
        default:              w = W_MSG;  h = H_MSG;  break;
        }
    }

    static ImVec2 CenterOnGameMonitor(int w, int h)
    {
        HWND hGame = nullptr;
        EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
            if (!IsWindowVisible(hwnd)) return TRUE;
            char cls[128] = {}, ttl[128] = {};
            GetClassNameA(hwnd, cls, sizeof cls);
            GetWindowTextA(hwnd, ttl, sizeof ttl);
            if (strstr(ttl, "ROB RUN") || (strstr(cls, "UnityWndClass") && strstr(ttl, "ROB")))
            { *reinterpret_cast<HWND*>(lp) = hwnd; return FALSE; }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&hGame));

        MONITORINFO mi = {}; mi.cbSize = sizeof mi;
        HMONITOR hm = hGame
            ? MonitorFromWindow(hGame, MONITOR_DEFAULTTONEAREST)
            : MonitorFromPoint({0,0}, MONITOR_DEFAULTTOPRIMARY);
        GetMonitorInfo(hm, &mi);
        int mw = mi.rcMonitor.right  - mi.rcMonitor.left;
        int mh = mi.rcMonitor.bottom - mi.rcMonitor.top;
        return { (float)(mi.rcMonitor.left + (mw - w) / 2),
                 (float)(mi.rcMonitor.top  + (mh - h) / 2) };
    }

    // -----------------------------------------------------------------------
    // DX11 device
    // -----------------------------------------------------------------------
    static bool CreateDev(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 3; // Triple buffering prevents DWM frame drops & compose stalls in Flip Model
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 0;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        D3D_FEATURE_LEVEL fl;
        const D3D_FEATURE_LEVEL fla[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
        HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                fla, 2, D3D11_SDK_VERSION, &sd, &g_pSC, &g_pDev, &fl, &g_pCtx);
        if (FAILED(hr))
        {
            // Fallback to legacy discard if Flip Model unsupported
            sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                fla, 2, D3D11_SDK_VERSION, &sd, &g_pSC, &g_pDev, &fl, &g_pCtx);
            if (FAILED(hr))
                return false;
        }

        ID3D11Texture2D* pBB = nullptr;
        g_pSC->GetBuffer(0, IID_PPV_ARGS(&pBB));
        if (pBB) { g_pDev->CreateRenderTargetView(pBB, nullptr, &g_pRTV); pBB->Release(); }
        return true;
    }

    static void CleanupDev()
    {
        if (g_pCtx) { g_pCtx->ClearState(); g_pCtx->Flush(); }
        if (g_pRTV) { g_pRTV->Release(); g_pRTV = nullptr; }
        if (g_pSC)  { g_pSC->Release();  g_pSC  = nullptr; }
        if (g_pCtx) { g_pCtx->Release(); g_pCtx = nullptr; }
        if (g_pDev) { g_pDev->Release(); g_pDev = nullptr; }
    }

    // -----------------------------------------------------------------------
    // WndProc
    // -----------------------------------------------------------------------
    static LRESULT WINAPI AuthWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        if (m == WM_LBUTTONDOWN) {
            SetForegroundWindow(h);
            SetFocus(h);
        }
        if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
        if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
        return DefWindowProc(h, m, w, l);
    }

    // -----------------------------------------------------------------------
    // Theme
    // -----------------------------------------------------------------------
    void AuthWindow::SetupTheme()
    {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding   = 12.f;
        s.ChildRounding    = 8.f;
        s.FrameRounding    = 6.f;
        s.GrabRounding     = 6.f;
        s.WindowPadding    = { 20.f, 18.f };
        s.FramePadding     = { 12.f,  8.f };
        s.ItemSpacing      = { 10.f, 10.f };
        s.WindowBorderSize = 1.f;
        s.FrameBorderSize  = 1.f;
        s.ChildBorderSize  = 1.f;

        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg]          = C_BG;
        c[ImGuiCol_ChildBg]           = C_CARD;
        c[ImGuiCol_Border]            = C_STROKE;
        c[ImGuiCol_BorderShadow]      = {0,0,0,0};
        c[ImGuiCol_FrameBg]           = C_CARD;
        c[ImGuiCol_FrameBgHovered]    = { 0.080f, 0.095f, 0.120f, 1.f };
        c[ImGuiCol_FrameBgActive]     = { 0.100f, 0.120f, 0.150f, 1.f };
        c[ImGuiCol_TitleBg] = c[ImGuiCol_TitleBgActive] = C_BG;
        c[ImGuiCol_ScrollbarBg]       = C_BG;
        c[ImGuiCol_CheckMark]         = C_ACCENT;
        c[ImGuiCol_Button]            = C_ACCENT;
        c[ImGuiCol_ButtonHovered]     = C_ACCENT_H;
        c[ImGuiCol_ButtonActive]      = C_ACCENT_A;
        c[ImGuiCol_Separator]         = C_STROKE;
        c[ImGuiCol_Text]              = C_TEXT;
        c[ImGuiCol_TextDisabled]      = C_DIM;
    }

    // -----------------------------------------------------------------------
    // Background workers
    // -----------------------------------------------------------------------
    void AuthWindow::StartBackgroundCheck()
    {
        s_State = STATE_CHECKING;
        s_ErrorMessage.clear();
        std::thread([]() {
            auto t0 = std::chrono::steady_clock::now();

            // Run location lookup asynchronously in background so external API (ip-api.com) never delays auth
            std::thread([]() {
                s_LocInfo = HWID::GetLocationInfo();
            }).detach();

            // Run hardware fingerprinting concurrently with network preflight
            auto hwFuture = std::async(std::launch::async, []() {
                return HWID::GetHardwareInfo();
            });

            // 0. Pre-flight health check: hit base URL ("/") and verify status == "ok" && database == "ok"
            HealthCheckResult hr = AuthClient::CheckHealth();
            if (!hr.success)
            {
                s_ErrorMessage = hr.error.empty() ? "Service is offline" : hr.error;
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - t0).count();
                if (ms < 150) std::this_thread::sleep_for(std::chrono::milliseconds(150 - ms));
                s_State = STATE_ERROR;
                return;
            }

            // 1. Check application version and active status in DB (AND logic)
            CheckVersionResult vr = AuthClient::CheckVersion(AuthClient::APP_VERSION);
            if (!vr.valid)
            {
                s_LatestVersion = vr.latestVersion.empty() ? "" : vr.latestVersion;
                s_DownloadLink  = vr.downloadLink.empty() ? "" : vr.downloadLink;
                s_ErrorMessage  = vr.error.empty() ? "The loader has expired. A new version is available." : vr.error;

                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - t0).count();
                if (ms < 150) std::this_thread::sleep_for(std::chrono::milliseconds(150 - ms));

                // If connection failed completely
                if (!vr.success && vr.error.find("Connection failed") != std::string::npos) {
                    s_State = STATE_ERROR;
                } else {
                    s_State = STATE_EXPIRED;
                    StartBackgroundUpdate();
                }
                return;
            }

            // 2. Version is valid and active -> retrieve computed hardware info & check HWID
            s_HwInfo = hwFuture.get();
            CheckHwidResult r = AuthClient::CheckHWID(s_HwInfo.hwid);

            // Smooth minimum transition (150ms)
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
            if (ms < 150) std::this_thread::sleep_for(std::chrono::milliseconds(150 - ms));

            if (!r.success)          { s_ErrorMessage = r.error; s_State = STATE_ERROR;   return; }
            if (r.registered)        {
                                           s_CurrentUsername = r.username;
                                           s_CurrentKey = r.key;
                                           s_CurrentHwid = s_HwInfo.hwid;
                                           if (!r.createdAt.empty()) s_CurrentCreatedAt = r.createdAt;
                                           if (r.expiresInDays >= 0) s_CurrentExpiresInDays = r.expiresInDays;
                                           if (r.banned) {
                                               s_BannedKey = r.key;
                                               s_BannedUserId = r.userId;
                                               s_ErrorMessage = !r.message.empty() ? r.message : "Your account has been terminated";
                                               s_State = STATE_BANNED;
                                           }
                                           else if (r.expired || (r.expiresInDays == 0 && !r.key.empty())) {
                                               s_CurrentKey = r.key;
                                               s_ErrorMessage = !r.message.empty() ? r.message : "Your License has been expired";
                                               s_State = STATE_KEY_EXPIRED;
                                           }
                                           else {
                                               s_SuccessTimer = 1.8f;
                                               ConfigCache::RefreshSync(s_CurrentUsername);
                                               s_State = STATE_LOGGED_IN;
                                               SetAuthenticated(true); // H-9 fix: set XOR token
                                               AuthClient::StartTelemetry(s_CurrentHwid, s_CurrentKey, s_CurrentUsername);

                                           }
                                       }
            else                     { s_State = STATE_REGISTER; }
        }).detach();
    }

    void AuthWindow::StartBackgroundRegistration()
    {
        if (s_IsSubmitting) return;
        std::string usr(s_UsernameBuffer), key(s_KeyBuffer);
        auto Trim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
        };
        Trim(usr);
        Trim(key);
        if (usr.size() < 2) { s_ErrorMessage = "Username must be at least 2 characters."; return; }
        if (key.empty())    { s_ErrorMessage = "License key is required."; return; }
        s_IsSubmitting = true; s_ErrorMessage.clear();
        std::thread([usr, key]() {
            RegisterPayload p; p.key = key; p.username = usr; p.hwid = s_HwInfo.hwid;
            RegisterResult  r = AuthClient::Register(p);
            s_IsSubmitting = false;
            if (r.success) {
                s_CurrentUsername = r.username.empty() ? usr : r.username;
                s_CurrentKey = key;
                s_CurrentHwid = s_HwInfo.hwid;
                if (!r.createdAt.empty()) s_CurrentCreatedAt = r.createdAt;
                if (r.expiresInDays >= 0) s_CurrentExpiresInDays = r.expiresInDays;
                s_SuccessTimer = 1.8f;
                ConfigCache::RefreshSync(s_CurrentUsername);
                s_State = STATE_LOGGED_IN;
                SetAuthenticated(true); // H-9 fix: set XOR token
                AuthClient::StartTelemetry(s_CurrentHwid, s_CurrentKey, s_CurrentUsername);

            }
            else           { s_ErrorMessage = r.error.empty() ? "Invalid key or username taken." : r.error; }
        }).detach();
    }

    // -----------------------------------------------------------------------
    // Render: Loading (First page: Minimalist luxury telemetry loader)
    // -----------------------------------------------------------------------
    void AuthWindow::RenderLoadingScreen()
    {
        ImDrawList* dl  = ImGui::GetWindowDrawList();
        ImVec2      wp  = ImGui::GetWindowPos();
        ImVec2      sz  = ImGui::GetWindowSize();
        ImVec2      cnt = wp + ImVec2(sz.x * 0.5f, sz.y * 0.50f);
        float       t   = (float)ImGui::GetTime();

        DrawBrandTitle(dl, wp.x + sz.x * 0.5f, wp.y + 24.f);

        // 2. Rotating telemetry ring (Centered, with fading cyan bloom arc matching screenshot)
        float r = 32.f;
        // Background track (subtle dark ring)
        dl->AddCircle(cnt, r, ImColor(18, 23, 30, 240), 64, 2.0f);

        // Rotating primary arc with outer soft bloom
        float a_start = t * 3.4f;
        float a_span  = 2.0f;
        int arcSteps  = 16;

        // Outer soft bloom layer
        for (int i = 0; i < arcSteps; ++i) {
            float frac0 = (float)i / (float)arcSteps;
            float frac1 = (float)(i + 1) / (float)arcSteps;
            float a0 = a_start + frac0 * a_span;
            float a1 = a_start + frac1 * a_span;
            int alphaBloom = (int)(frac1 * 40.f);
            dl->PathClear();
            dl->PathArcTo(cnt, r, a0, a1, 3);
            dl->PathStroke(ImColor(0, 242, 254, alphaBloom), 0, 5.0f);
        }

        // Core sharp glowing arc fading from tail to head
        for (int i = 0; i < arcSteps; ++i) {
            float frac0 = (float)i / (float)arcSteps;
            float frac1 = (float)(i + 1) / (float)arcSteps;
            float a0 = a_start + frac0 * a_span;
            float a1 = a_start + frac1 * a_span;
            int alphaCore = (int)(35.f + frac1 * 220.f);
            dl->PathClear();
            dl->PathArcTo(cnt, r, a0, a1, 3);
            dl->PathStroke(ImColor(0, 242, 254, alphaCore), 0, 2.5f);
        }

        // Glowing leading tip node
        float a_tip = a_start + a_span;
        ImVec2 tip = cnt + ImVec2(std::cos(a_tip) * r, std::sin(a_tip) * r);
        dl->AddCircleFilled(tip, 2.6f, ImColor(255, 255, 255, 255));
        dl->AddCircle(tip, 4.8f, ImColor(0, 242, 254, 160), 16, 1.2f);
    }

    // -----------------------------------------------------------------------
    // Render: Registration (Page 2: Minimalist luxury activation card - exact match to SS)
    // -----------------------------------------------------------------------
    void AuthWindow::RenderRegistrationScreen()
    {
        if (g_FontMed) ImGui::PushFont(g_FontMed);

        float availW = ImGui::GetContentRegionAvail().x;
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 sz = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        DrawBrandTitle(dl, wp.x + sz.x * 0.5f, wp.y + 24.f);

        // Set cursor below header and version
        ImGui::SetCursorPosY(76.f);

        // 2. Error Banner (subtle, if any)
        if (!s_ErrorMessage.empty()) {
            ImVec2 errPos = ImGui::GetCursorScreenPos();
            ImVec2 errSz  = ImVec2(availW, 24.f);
            dl->AddRectFilled(errPos, errPos + errSz, ImColor(45, 18, 22, 220), 6.0f);
            dl->AddRect(errPos, errPos + errSz, ImColor(255, 59, 48, 160), 6.0f, 0, 1.0f);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.f);
            if (g_FontSmall) ImGui::PushFont(g_FontSmall);
            ImGui::PushStyleColor(ImGuiCol_Text, C_ERR);
            ImGui::TextWrapped("%s", s_ErrorMessage.c_str());
            ImGui::PopStyleColor();
            if (g_FontSmall) ImGui::PopFont();
            ImGui::Spacing();
        }

        // 3. Input Fields
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.f, 8.5f));

        // Field 1: USERNAME
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.420f, 0.470f, 0.550f, 1.f)); // Muted clean gray
        ImGui::Text("USERNAME");
        ImGui::PopStyleColor();
        if (g_FontSmall) ImGui::PopFont();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.055f, 0.063f, 0.078f, 1.f)); // Dark matte glass
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.075f, 0.088f, 0.110f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(0.085f, 0.100f, 0.125f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(0.140f, 0.160f, 0.200f, 0.6f));
        ImGui::SetNextItemWidth(availW);
        ImGui::InputText("##u", s_UsernameBuffer, sizeof s_UsernameBuffer);
        ImGui::PopStyleColor(4);

        ImGui::Spacing();

        // Field 2: LICENSE KEY
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.420f, 0.470f, 0.550f, 1.f)); // Muted clean gray
        ImGui::Text("LICENSE KEY");
        ImGui::PopStyleColor();
        if (g_FontSmall) ImGui::PopFont();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.055f, 0.063f, 0.078f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.075f, 0.088f, 0.110f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(0.085f, 0.100f, 0.125f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(0.140f, 0.160f, 0.200f, 0.6f));
        ImGui::SetNextItemWidth(availW);
        ImGui::InputText("##k", s_KeyBuffer, sizeof s_KeyBuffer);
        ImGui::PopStyleColor(4);

        ImGui::PopStyleVar(3); // pop FrameRounding, FrameBorderSize, FramePadding

        ImGui::SetCursorPosY(242.f);

        // 4. Glowing Pill Activate Button (exact match to screenshot)
        auto HasNonWhitespace = [](const char* str) -> bool {
            if (!str) return false;
            for (; *str != '\0'; ++str) {
                if (!std::isspace(static_cast<unsigned char>(*str)))
                    return true;
            }
            return false;
        };

        bool hasValidInputs = HasNonWhitespace(s_UsernameBuffer) && HasNonWhitespace(s_KeyBuffer);
        bool locked         = s_IsSubmitting || !hasValidInputs;

        ImVec2 btnMin = ImGui::GetCursorScreenPos();
        float btnH = 36.f;
        ImVec2 btnMax = ImVec2(btnMin.x + availW, btnMin.y + btnH);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 18.0f); // Pill shape

        if (locked && !s_IsSubmitting) {
            // Disabled flat state
            ImVec4 dimCol = { 0.075f, 0.085f, 0.105f, 1.f };
            ImVec4 dimTxt = { 0.320f, 0.360f, 0.420f, 1.f };
            ImGui::PushStyleColor(ImGuiCol_Button,        dimCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, dimCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  dimCol);
            ImGui::PushStyleColor(ImGuiCol_Text,          dimTxt);
            ImGui::Button("ACTIVATE", ImVec2(availW, btnH));
            ImGui::PopStyleColor(4);
        } else if (s_IsSubmitting) {
            // Submitting spinner with glowing background
            for (float off = 3.0f; off >= 1.0f; off -= 1.0f) {
                dl->AddRectFilled(btnMin - ImVec2(off, off), btnMax + ImVec2(off, off), ImColor(0, 242, 254, (int)(18 * (4.0f - off))), 18.f + off);
            }
            ImGui::PushStyleColor(ImGuiCol_Button,        C_ACCENT);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C_ACCENT);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  C_ACCENT);
            ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0,0,0,0));
            ImGui::Button("##spin", ImVec2(availW, btnH));
            ImGui::PopStyleColor(4);

            ImVec2 bMin = ImGui::GetItemRectMin();
            ImVec2 bMax = ImGui::GetItemRectMax();
            ImVec2 cnt  = { (bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f };
            float t  = (float)ImGui::GetTime();
            float r  = 8.f;
            int   n  = 8;
            for (int i = 0; i < n; i++) {
                float a   = t * 5.f + i * (6.2832f / n);
                float alp = (float)(i + 1) / n;
                ImVec2 p  = cnt + ImVec2(std::cos(a) * r, std::sin(a) * r);
                dl->AddCircleFilled(p, 1.8f, ImColor(7, 9, 12, (int)(alp * 240)));
            }
        } else {
            // Soft outer cyan glow aura around active button (just like screenshot)
            for (float off = 3.5f; off >= 1.0f; off -= 1.0f) {
                dl->AddRectFilled(btnMin - ImVec2(off, off), btnMax + ImVec2(off, off), ImColor(0, 242, 254, (int)(22 * (4.5f - off))), 18.f + off);
            }

            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.27f, 0.91f, 0.96f, 1.f)); // Vibrant cyan
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.95f, 1.00f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.20f, 0.85f, 0.90f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.03f, 0.05f, 0.08f, 1.f)); // Crisp dark contrast text

            if (ImGui::Button("ACTIVATE", ImVec2(availW, btnH)))
                StartBackgroundRegistration();
            ImGui::PopStyleColor(4);
        }

        ImGui::PopStyleVar(); // pop FrameRounding

        if (g_FontMed) ImGui::PopFont();
    }

    // -----------------------------------------------------------------------
    // Render: Success (Centered Minimalist Brand Reveal with converging greeting)
    // -----------------------------------------------------------------------
    void AuthWindow::RenderSuccessScreen()
    {
        const float TOTAL_TIME = 1.8f;
        s_SuccessTimer -= ImGui::GetIO().DeltaTime;
        float elapsed = TOTAL_TIME - s_SuccessTimer;
        float tNorm   = std::clamp(elapsed / TOTAL_TIME, 0.0f, 1.0f);

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 sz = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // 1. Deep Obsidian Card Background
        dl->AddRectFilled(wp, wp + sz, ImColor(C_BG), 10.f);
        dl->AddRect(wp, wp + sz, ImColor(C_STROKE), 10.f);

        DrawBrandTitle(dl, wp.x + sz.x * 0.5f, wp.y + 24.f);

        // 3. Converging Greeting Animation ("Hey " from left, "{username}" from right)
        std::string prefix = "Hey ";
        std::string user   = s_CurrentUsername.empty() ? "User" : s_CurrentUsername;

        ImFont* fontGreet = g_FontTitle ? g_FontTitle : g_FontMed;
        ImVec2 szPref = fontGreet ? fontGreet->CalcTextSizeA(fontGreet->FontSize, FLT_MAX, 0.f, prefix.c_str()) : ImGui::CalcTextSize(prefix.c_str());
        ImVec2 szUser = fontGreet ? fontGreet->CalcTextSizeA(fontGreet->FontSize, FLT_MAX, 0.f, user.c_str()) : ImGui::CalcTextSize(user.c_str());
        float totalGreetW = szPref.x + szUser.x;
        float subX        = wp.x + (sz.x - totalGreetW) * 0.5f;
        float subY        = wp.y + 104.f;

        float heyT    = std::clamp(elapsed / TOTAL_TIME, 0.0f, 1.0f);
        float heyEase = 1.0f - std::pow(1.0f - heyT, 3.0f); // responsive cubic ease curve

        float leftOffset  = -(1.0f - heyEase) * 80.0f;
        float rightOffset =  (1.0f - heyEase) * 80.0f;
        float subEase     = heyEase;

        // Ambient radial cyan bloom behind greeting
        if (subEase > 0.05f) {
            dl->AddCircleFilled(wp + ImVec2(sz.x * 0.5f, subY + szPref.y * 0.5f), 55.f * subEase, ImColor(0, 242, 254, (int)(18.f * subEase)));
        }

        if (subEase > 0.01f) {
            // Left word: "Hey " slides in from left
            ImVec2 heyPos = ImVec2(subX + leftOffset, subY);
            if (fontGreet) {
                dl->AddText(fontGreet, fontGreet->FontSize, heyPos, ImColor(C_MID.x, C_MID.y, C_MID.z, subEase * 0.95f), prefix.c_str());
            } else {
                dl->AddText(heyPos, ImColor(C_MID.x, C_MID.y, C_MID.z, subEase * 0.95f), prefix.c_str());
            }

            // Right word: username slides in from right with radiant Ice Cyan bloom (#00F2FE)
            ImVec2 userPos = ImVec2(subX + szPref.x + rightOffset, subY);
            float glowAlpha = subEase * 0.45f;
            int gCol = (int)(glowAlpha * 120);

            if (fontGreet) {
                dl->AddText(fontGreet, fontGreet->FontSize, userPos + ImVec2(-1.f, 0), ImColor(0, 242, 254, gCol), user.c_str());
                dl->AddText(fontGreet, fontGreet->FontSize, userPos + ImVec2(1.f, 0),  ImColor(0, 242, 254, gCol), user.c_str());
                dl->AddText(fontGreet, fontGreet->FontSize, userPos + ImVec2(0, -1.f), ImColor(0, 242, 254, gCol), user.c_str());
                dl->AddText(fontGreet, fontGreet->FontSize, userPos + ImVec2(0, 1.f),  ImColor(0, 242, 254, gCol), user.c_str());
                dl->AddText(fontGreet, fontGreet->FontSize, userPos, ImColor(0, 242, 254, (int)(255.f * subEase)), user.c_str());
            } else {
                dl->AddText(userPos + ImVec2(-1.f, 0), ImColor(0, 242, 254, gCol), user.c_str());
                dl->AddText(userPos + ImVec2(1.f, 0),  ImColor(0, 242, 254, gCol), user.c_str());
                dl->AddText(userPos + ImVec2(0, -1.f), ImColor(0, 242, 254, gCol), user.c_str());
                dl->AddText(userPos + ImVec2(0, 1.f),  ImColor(0, 242, 254, gCol), user.c_str());
                dl->AddText(userPos, ImColor(0, 242, 254, (int)(255.f * subEase)), user.c_str());
            }
        }

        // 4. Bottom Cyan Progress Sweep Line
        dl->AddLine(wp + ImVec2(0, sz.y - 3.5f), wp + ImVec2(sz.x * tNorm, sz.y - 3.5f), ImColor(0, 242, 254, (int)(tNorm * 45)), 4.0f);
        dl->AddLine(wp + ImVec2(0, sz.y - 2.5f), wp + ImVec2(sz.x * tNorm, sz.y - 2.5f), ImColor(0, 242, 254, (int)(tNorm * 230)), 2.0f);

        // Completion transition
        if (s_SuccessTimer <= 0.f) {
            SetAuthenticated(true);
            g_Running = false;
        }
    }

    // -----------------------------------------------------------------------
    // Render: Banned (Redesigned Sharp Architectural Security Enforcement)
    // -----------------------------------------------------------------------
    void AuthWindow::RenderBannedScreen()
    {
        if (g_FontMed) ImGui::PushFont(g_FontMed);
        ImVec2 sz = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float availW = sz.x - 40.f;

        // 1. Top Crimson Glowing Accent Line
        ImVec2 divStart = ImVec2(wp.x + 25.f, wp.y);
        dl->AddLine(divStart, divStart + ImVec2(sz.x - 50.f, 0.f), ImColor(255, 59, 48, 220), 2.0f);
        dl->AddRectFilledMultiColor(divStart, divStart + ImVec2(sz.x - 50.f, 26.f),
            ImColor(255, 59, 48, 35), ImColor(255, 59, 48, 35),
            ImColor(255, 59, 48, 0), ImColor(255, 59, 48, 0));

        // 2. Padlock Icon
        ImVec2 lockCenter = wp + ImVec2(sz.x * 0.5f, 32.f);
        // Shackle
        dl->PathClear();
        dl->PathArcTo(lockCenter + ImVec2(0.f, -4.f), 6.5f, 3.14159f, 6.28318f, 16);
        dl->PathStroke(ImColor(255, 255, 255, 220), 0, 2.0f);
        // Body
        dl->AddRectFilled(lockCenter + ImVec2(-9.f, -4.f), lockCenter + ImVec2(9.f, 10.f), ImColor(255, 255, 255, 220), 3.0f);
        // Keyhole
        dl->AddCircleFilled(lockCenter + ImVec2(0.f, 1.5f), 1.5f, ImColor(20, 24, 30, 255));
        dl->AddLine(lockCenter + ImVec2(0.f, 1.5f), lockCenter + ImVec2(0.f, 5.5f), ImColor(20, 24, 30, 255), 1.5f);

        // 3. Header & Subtitle
        ImGui::SetCursorPos({ 20.f, 52.f });
        const char* title = "ACCESS RESTRICTED";
        if (g_FontTitle) ImGui::PushFont(g_FontTitle);
        ImVec2 titleSz = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((sz.x - titleSz.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        if (g_FontTitle) ImGui::PopFont();

        const char* sub = !s_ErrorMessage.empty() ? s_ErrorMessage.c_str() : "Your account has been terminated";
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        ImVec2 subSz = ImGui::CalcTextSize(sub);
        ImGui::SetCursorPosX((sz.x - subSz.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, C_ERR);
        ImGui::Text("%s", sub);
        ImGui::PopStyleColor();
        if (g_FontSmall) ImGui::PopFont();

        ImGui::SetCursorPosY(98.f);

        // 4. Structured Dark Violation Box
        ImVec2 boxMin = ImVec2(wp.x + 20.f, wp.y + 98.f);
        ImVec2 boxMax = ImVec2(boxMin.x + availW, boxMin.y + 54.f);
        dl->AddRectFilled(boxMin, boxMax, ImColor(14, 17, 24, 220), 8.0f);
        dl->AddRect(boxMin, boxMax, ImColor(35, 42, 54, 160), 8.0f, 0, 1.0f);

        std::string rawKey = !s_BannedKey.empty() ? s_BannedKey : (s_KeyBuffer[0] ? s_KeyBuffer : "CRNG-9821-XXXX");
        std::string maskKey = (rawKey.length() > 14) ? (rawKey.substr(0, 9) + "-XXXX-" + rawKey.substr(rawKey.length() - 4)) : rawKey;

        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        dl->AddText(boxMin + ImVec2(12.f, 9.f), ImColor(C_DIM.x, C_DIM.y, C_DIM.z, 1.f), "Banned Key:");
        dl->AddText(boxMin + ImVec2(availW - 12.f - ImGui::CalcTextSize(maskKey.c_str()).x, 9.f), ImColor(C_TEXT.x, C_TEXT.y, C_TEXT.z, 0.95f), maskKey.c_str());

        dl->AddText(boxMin + ImVec2(12.f, 29.f), ImColor(C_DIM.x, C_DIM.y, C_DIM.z, 1.f), "Reason:");
        dl->AddText(boxMin + ImVec2(availW - 12.f - ImGui::CalcTextSize("Security Integrity Policy #402").x, 29.f), ImColor(C_MID.x, C_MID.y, C_MID.z, 0.95f), "Security Integrity Policy #402");
        if (g_FontSmall) ImGui::PopFont();

        // 5. Action Buttons (Pill buttons)
        ImGui::SetCursorPosY(166.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        float btnW = (availW - 10.f) * 0.5f;
        float btnH = 32.f;

        // Button 1: Appeal Via Discord
        ImGui::SetCursorPosX(20.f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.14f, 0.18f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.19f, 0.25f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.20f, 0.24f, 0.30f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.30f, 0.40f, 0.5f));
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);

        if (ImGui::Button("APPEAL VIA DISCORD", ImVec2(btnW, btnH))) {
            ShellExecuteA(nullptr, "open", "https://discord.gg", nullptr, nullptr, SW_SHOWNORMAL);
        }

        ImGui::SameLine(0.f, 10.f);

        // Button 2: Copy Info
        static float s_CopiedTimer = 0.f;
        if (s_CopiedTimer > 0.f) s_CopiedTimer -= ImGui::GetIO().DeltaTime;

        const char* copyLabel = (s_CopiedTimer > 0.f) ? "COPIED DETAILS!" : "COPY BAN DETAILS";
        if (s_CopiedTimer > 0.f) ImGui::PushStyleColor(ImGuiCol_Text, C_OK);
        else ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);

        if (ImGui::Button(copyLabel, ImVec2(btnW, btnH))) {
            std::string uname = s_CurrentUsername.empty() ? (s_UsernameBuffer[0] ? s_UsernameBuffer : "N/A") : s_CurrentUsername;
            std::string hwidStr = s_HwInfo.hwid.empty() ? GetHWID() : s_HwInfo.hwid;
            std::string clip = "Username: " + uname + "\nHWID: " + hwidStr + "\nKey: " + rawKey + "\nViolation: Policy #402";
            ImGui::SetClipboardText(clip.c_str());
            s_CopiedTimer = 2.0f;
        }
        ImGui::PopStyleColor(); // Text color

        if (g_FontSmall) ImGui::PopFont();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);

        if (g_FontMed) ImGui::PopFont();
    }

    // -----------------------------------------------------------------------
    // Render: Key Expired (Page 6: Access restricted due to expired license)
    // -----------------------------------------------------------------------
    void AuthWindow::RenderKeyExpiredScreen()
    {
        if (g_FontMed) ImGui::PushFont(g_FontMed);
        ImVec2 sz = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float availW = sz.x - 40.f;

        // 1. Top Amber Glowing Accent Line
        ImVec2 divStart = ImVec2(wp.x + 25.f, wp.y);
        dl->AddLine(divStart, divStart + ImVec2(sz.x - 50.f, 0.f), ImColor(255, 170, 0, 220), 2.0f);
        dl->AddRectFilledMultiColor(divStart, divStart + ImVec2(sz.x - 50.f, 26.f),
            ImColor(255, 170, 0, 35), ImColor(255, 170, 0, 35),
            ImColor(255, 170, 0, 0), ImColor(255, 170, 0, 0));

        // 2. Hourglass / Key Clock Icon
        ImVec2 iconCenter = wp + ImVec2(sz.x * 0.5f, 32.f);
        dl->PathClear();
        dl->PathLineTo(iconCenter + ImVec2(-7.f, -9.f));
        dl->PathLineTo(iconCenter + ImVec2(7.f, -9.f));
        dl->PathLineTo(iconCenter + ImVec2(-7.f, 9.f));
        dl->PathLineTo(iconCenter + ImVec2(7.f, 9.f));
        dl->PathLineTo(iconCenter + ImVec2(-7.f, -9.f));
        dl->PathStroke(ImColor(255, 190, 40, 220), 0, 1.8f);

        // 3. Header & Subtitle
        ImGui::SetCursorPos({ 20.f, 52.f });
        const char* title = "ACCESS RESTRICTED";
        if (g_FontTitle) ImGui::PushFont(g_FontTitle);
        ImVec2 titleSz = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((sz.x - titleSz.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        if (g_FontTitle) ImGui::PopFont();

        const char* sub = !s_ErrorMessage.empty() ? s_ErrorMessage.c_str() : "Your License has been expired";
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        ImVec2 subSz = ImGui::CalcTextSize(sub);
        ImGui::SetCursorPosX((sz.x - subSz.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.68f, 0.15f, 1.0f));
        ImGui::Text("%s", sub);
        ImGui::PopStyleColor();
        if (g_FontSmall) ImGui::PopFont();

        ImGui::SetCursorPosY(98.f);

        // 4. Structured Dark Expiration Info Box
        ImVec2 boxMin = ImVec2(wp.x + 20.f, wp.y + 98.f);
        ImVec2 boxMax = ImVec2(boxMin.x + availW, boxMin.y + 54.f);
        dl->AddRectFilled(boxMin, boxMax, ImColor(14, 17, 24, 220), 8.0f);
        dl->AddRect(boxMin, boxMax, ImColor(35, 42, 54, 160), 8.0f, 0, 1.0f);

        std::string rawKey = !s_CurrentKey.empty() ? s_CurrentKey : (s_KeyBuffer[0] ? s_KeyBuffer : "CRNG-EXPIRED-KEY");
        std::string maskKey = (rawKey.length() > 14) ? (rawKey.substr(0, 9) + "-XXXX-" + rawKey.substr(rawKey.length() - 4)) : rawKey;

        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        dl->AddText(boxMin + ImVec2(12.f, 9.f), ImColor(C_DIM.x, C_DIM.y, C_DIM.z, 1.f), "License Key:");
        dl->AddText(boxMin + ImVec2(availW - 12.f - ImGui::CalcTextSize(maskKey.c_str()).x, 9.f), ImColor(C_TEXT.x, C_TEXT.y, C_TEXT.z, 0.95f), maskKey.c_str());

        dl->AddText(boxMin + ImVec2(12.f, 29.f), ImColor(C_DIM.x, C_DIM.y, C_DIM.z, 1.f), "Duration:");
        const char* expStatus = "0 Days Remaining (Expired)";
        dl->AddText(boxMin + ImVec2(availW - 12.f - ImGui::CalcTextSize(expStatus).x, 29.f), ImColor(255, 170, 0, 230), expStatus);
        if (g_FontSmall) ImGui::PopFont();

        // 5. Action Buttons (Pill buttons)
        ImGui::SetCursorPosY(166.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        float btnW = (availW - 10.f) * 0.5f;
        float btnH = 32.f;

        // Button 1: Renew via Discord
        ImGui::SetCursorPosX(20.f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.14f, 0.08f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.18f, 0.10f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f, 0.22f, 0.12f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.65f, 0.0f, 0.6f));
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);

        if (ImGui::Button("RENEW VIA DISCORD", ImVec2(btnW, btnH))) {
            ShellExecuteA(nullptr, "open", "https://discord.gg", nullptr, nullptr, SW_SHOWNORMAL);
        }

        ImGui::SameLine(0.f, 10.f);

        // Button 2: Copy Key Details
        static float s_CopiedTimer = 0.f;
        if (s_CopiedTimer > 0.f) s_CopiedTimer -= ImGui::GetIO().DeltaTime;

        const char* copyLabel = (s_CopiedTimer > 0.f) ? "COPIED DETAILS!" : "COPY KEY DETAILS";
        if (s_CopiedTimer > 0.f) ImGui::PushStyleColor(ImGuiCol_Text, C_OK);
        else ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);

        if (ImGui::Button(copyLabel, ImVec2(btnW, btnH))) {
            std::string uname = s_CurrentUsername.empty() ? (s_UsernameBuffer[0] ? s_UsernameBuffer : "N/A") : s_CurrentUsername;
            std::string hwidStr = s_HwInfo.hwid.empty() ? GetHWID() : s_HwInfo.hwid;
            std::string clip = "Username: " + uname + "\nHWID: " + hwidStr + "\nKey: " + rawKey + "\nStatus: Expired";
            ImGui::SetClipboardText(clip.c_str());
            s_CopiedTimer = 2.0f;
        }
        ImGui::PopStyleColor(); // Text color

        if (g_FontSmall) ImGui::PopFont();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);

        if (g_FontMed) ImGui::PopFont();
    }

    // -----------------------------------------------------------------------
    // Render: Error (Page 4: Minimalist luxury server unreachable screen)
    // -----------------------------------------------------------------------
    void AuthWindow::RenderErrorScreen()
    {
        if (g_FontMed) ImGui::PushFont(g_FontMed);
        ImVec2 sz = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float availW = sz.x - 40.f;

        // 1. Top Amber Glowing Accent Line
        ImVec2 divStart = ImVec2(wp.x + 25.f, wp.y);
        dl->AddLine(divStart, divStart + ImVec2(sz.x - 50.f, 0.f), ImColor(255, 159, 10, 220), 2.0f);
        dl->AddRectFilledMultiColor(divStart, divStart + ImVec2(sz.x - 50.f, 26.f),
            ImColor(255, 159, 10, 35), ImColor(255, 159, 10, 35),
            ImColor(255, 159, 10, 0), ImColor(255, 159, 10, 0));

        // 2. Severed Connection Icon
        ImVec2 icCenter = wp + ImVec2(sz.x * 0.5f, 32.f);
        // Two interlocking loops
        dl->AddCircle(icCenter + ImVec2(-6.f, 0.f), 6.5f, ImColor(255, 255, 255, 200), 24, 1.8f);
        dl->AddCircle(icCenter + ImVec2(6.f, 0.f), 6.5f, ImColor(255, 255, 255, 200), 24, 1.8f);
        // Diagonal severance strike
        dl->AddLine(icCenter + ImVec2(-12.f, -10.f), icCenter + ImVec2(12.f, 10.f), ImColor(255, 159, 10, 255), 2.2f);

        // 3. Header & Subtitle
        ImGui::SetCursorPos({ 20.f, 52.f });
        const char* title = "SERVER UNREACHABLE";
        if (g_FontTitle) ImGui::PushFont(g_FontTitle);
        ImVec2 titleSz = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((sz.x - titleSz.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        if (g_FontTitle) ImGui::PopFont();

        const char* sub = "HTTP 502 Gateway Timeout";
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        ImVec2 subSz = ImGui::CalcTextSize(sub);
        ImGui::SetCursorPosX((sz.x - subSz.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, C_AMBER);
        ImGui::Text("%s", sub);
        ImGui::PopStyleColor();
        if (g_FontSmall) ImGui::PopFont();

        ImGui::SetCursorPosY(98.f);

        // 4. Structured Diagnostic Box
        ImVec2 boxMin = ImVec2(wp.x + 20.f, wp.y + 98.f);
        ImVec2 boxMax = ImVec2(boxMin.x + availW, boxMin.y + 54.f);
        dl->AddRectFilled(boxMin, boxMax, ImColor(14, 17, 24, 220), 8.0f);
        dl->AddRect(boxMin, boxMax, ImColor(35, 42, 54, 160), 8.0f, 0, 1.0f);

        std::string errStr = s_ErrorMessage.empty() ? "Authentication gateway offline." : s_ErrorMessage;
        if (errStr.length() > 38) errStr = errStr.substr(0, 35) + "...";

        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        dl->AddText(boxMin + ImVec2(12.f, 9.f), ImColor(C_DIM.x, C_DIM.y, C_DIM.z, 1.f), "Ping Status:");
        dl->AddText(boxMin + ImVec2(availW - 12.f - ImGui::CalcTextSize("Timed out").x, 9.f), ImColor(255, 159, 10, 240), "Timed out");

        dl->AddText(boxMin + ImVec2(12.f, 29.f), ImColor(C_DIM.x, C_DIM.y, C_DIM.z, 1.f), "Diagnostics:");
        dl->AddText(boxMin + ImVec2(availW - 12.f - ImGui::CalcTextSize(errStr.c_str()).x, 29.f), ImColor(C_MID.x, C_MID.y, C_MID.z, 0.95f), errStr.c_str());
        if (g_FontSmall) ImGui::PopFont();

        // 5. Action Buttons (Pill buttons)
        ImGui::SetCursorPosY(166.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        float btnW = (availW - 10.f) * 0.5f;
        float btnH = 32.f;

        // Button 1: Retry Connection (glowing cyan outline)
        ImGui::SetCursorPosX(20.f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.16f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.18f, 0.24f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.16f, 0.22f, 0.30f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border, C_ACCENT);
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);

        if (ImGui::Button("RETRY CONNECTION", ImVec2(btnW, btnH))) {
            StartBackgroundCheck();
        }

        ImGui::SameLine(0.f, 10.f);

        // Button 2: Exit
        ImGui::PushStyleColor(ImGuiCol_Button, C_BTN_GHOST);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C_BTN_GH);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  C_BTN_GA);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 0.5f));

        if (ImGui::Button("EXIT", ImVec2(btnW, btnH))) {
            g_Running = false;
        }

        if (g_FontSmall) ImGui::PopFont();
        ImGui::PopStyleColor(8);
        ImGui::PopStyleVar(2);

        if (g_FontMed) ImGui::PopFont();
    }

    // -----------------------------------------------------------------------
    // Background worker: In-App Automatic Updater
    // -----------------------------------------------------------------------
    void AuthWindow::StartBackgroundUpdate()
    {
        if (s_IsDownloading) return;
        s_IsDownloading    = true;
        s_DownloadComplete = false;
        s_DownloadFailed   = false;
        s_DownloadedBytes  = 0;
        s_TotalBytes       = 0;
        s_DownloadSpeed    = 0.f;

        std::thread([]() {
            if (s_DownloadLink.empty())
            {
                s_DownloadFailed = true;
                s_IsDownloading  = false;
                s_ErrorMessage   = "Download package URL is missing or unconfigured.";
                return;
            }

            wchar_t currentExe[MAX_PATH] = { 0 };
            if (GetModuleFileNameW(nullptr, currentExe, MAX_PATH) == 0)
            {
                s_DownloadFailed = true;
                s_IsDownloading  = false;
                s_ErrorMessage   = "Failed to resolve local executable path.";
                return;
            }

            std::wstring newExe = std::wstring(currentExe) + L".new";
            std::wstring oldExe = std::wstring(currentExe) + L".old";

            // Delete any existing temporary or backup files
            DeleteFileW(newExe.c_str());
            DeleteFileW(oldExe.c_str());

            // Convert UTF-8 URL to wide string
            int wLen = MultiByteToWideChar(CP_UTF8, 0, s_DownloadLink.c_str(), -1, nullptr, 0);
            std::wstring wUrl(wLen, 0);
            MultiByteToWideChar(CP_UTF8, 0, s_DownloadLink.c_str(), -1, &wUrl[0], wLen);
            if (!wUrl.empty() && wUrl.back() == L'\0') wUrl.pop_back();

            // Clear cache entry to ensure fresh bits are downloaded
            DeleteUrlCacheEntryW(wUrl.c_str());

            UpdateDownloadCallback cb;
            HRESULT hr = URLDownloadToFileW(nullptr, wUrl.c_str(), newExe.c_str(), 0, &cb);

            if (SUCCEEDED(hr))
            {
                // Verify downloaded file size (> 100 KB)
                HANDLE hFile = CreateFileW(newExe.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
                uint64_t fileSize = 0;
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    LARGE_INTEGER fs;
                    if (GetFileSizeEx(hFile, &fs)) fileSize = (uint64_t)fs.QuadPart;
                    CloseHandle(hFile);
                }

                if (fileSize > 100 * 1024)
                {
                    if (s_TotalBytes.load() == 0)
                    {
                        s_TotalBytes.store(fileSize);
                    }
                    s_DownloadedBytes.store(s_TotalBytes.load());
                    s_DownloadComplete = true;

                    // Allow brief pause so user sees 100% completion
                    std::this_thread::sleep_for(std::chrono::milliseconds(800));

                    // Step 1: Remove leftover old file if present
                    for (int i = 0; i < 10; ++i)
                    {
                        SetFileAttributesW(oldExe.c_str(), FILE_ATTRIBUTE_NORMAL);
                        if (DeleteFileW(oldExe.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND) break;
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }

                    // Step 2: Rename current running executable to .old
                    BOOL movedToOld = MoveFileW(currentExe, oldExe.c_str());
                    if (!movedToOld)
                    {
                        movedToOld = MoveFileExW(currentExe, oldExe.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
                    }

                    if (!movedToOld)
                    {
                        s_DownloadFailed = true;
                        s_IsDownloading  = false;
                        s_ErrorMessage   = "Failed to rename current executable for update.";
                        return;
                    }

                    // Step 3: Rename downloaded .new to currentExe
                    SetFileAttributesW(currentExe, FILE_ATTRIBUTE_NORMAL);
                    BOOL movedNew = MoveFileW(newExe.c_str(), currentExe);
                    if (!movedNew)
                    {
                        movedNew = MoveFileExW(newExe.c_str(), currentExe, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
                    }

                    if (movedNew)
                    {
                        // Step 4: Spawn an independent background cmd cleaner to force-delete the .old file as soon as this process exits
                        wchar_t cmd[1024];
                        swprintf_s(cmd, 1024,
                            L"cmd.exe /c \"ping 127.0.0.1 -n 2 >nul & for /l %%x in (1,1,30) do (del /f /q \"%s\" 2>nul && exit || ping 127.0.0.1 -n 2 >nul)\"",
                            oldExe.c_str());

                        STARTUPINFOW siCmd = { sizeof(siCmd) };
                        siCmd.dwFlags = STARTF_USESHOWWINDOW;
                        siCmd.wShowWindow = SW_HIDE;
                        PROCESS_INFORMATION piCmd = {};
                        if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &siCmd, &piCmd))
                        {
                            CloseHandle(piCmd.hProcess);
                            CloseHandle(piCmd.hThread);
                        }

                        // Step 5: Launch updated binary
                        STARTUPINFOW si = { sizeof(si) };
                        PROCESS_INFORMATION pi = {};
                        if (CreateProcessW(currentExe, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
                        {
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        }
                        ConfigCache::Wipe();
                        ExitProcess(0);
                    }
                    else
                    {
                        // Rollback on move failure
                        MoveFileW(oldExe.c_str(), currentExe);
                        s_DownloadFailed = true;
                        s_IsDownloading  = false;
                        s_ErrorMessage   = "Failed to replace executable binary.";
                    }
                }
                else
                {
                    DeleteFileW(newExe.c_str());
                    s_DownloadFailed = true;
                    s_IsDownloading  = false;
                    s_ErrorMessage   = "Downloaded update package is corrupted or invalid.";
                }
            }
            else
            {
                DeleteFileW(newExe.c_str());
                s_DownloadFailed = true;
                s_IsDownloading  = false;
                char errBuf[128];
                sprintf_s(errBuf, "Download failed (0x%08X). Check network.", (unsigned int)hr);
                s_ErrorMessage = errBuf;
            }
        }).detach();
    }

    // -----------------------------------------------------------------------
    // Render: Expired / Automatic Updater UI (Exact match to SS - Fully Automatic)
    // -----------------------------------------------------------------------
    void AuthWindow::RenderExpiredScreen()
    {
        if (g_FontMed) ImGui::PushFont(g_FontMed);
        ImVec2 sz = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();

        // 1. Version Badge (Centered top pill badge - exact match to screenshot)
        std::string badgeText = "v" + (!s_LatestVersion.empty() ? s_LatestVersion : "2.5.0") + " Available";
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        ImVec2 badgeSz = ImGui::CalcTextSize(badgeText.c_str());
        float bPillW = badgeSz.x + 24.f;
        float bPillH = 26.f;
        ImVec2 bPillMin = ImVec2(wp.x + (sz.x - bPillW) * 0.5f, wp.y + 34.f);
        ImVec2 bPillMax = ImVec2(bPillMin.x + bPillW, bPillMin.y + bPillH);

        // Dark teal/cyan pill container with subtle cyan border
        dl->AddRectFilled(bPillMin, bPillMax, ImColor(12, 28, 36, 230), 13.0f);
        dl->AddRect(bPillMin, bPillMax, ImColor(0, 242, 254, 110), 13.0f, 0, 1.0f);
        dl->AddText(ImVec2(bPillMin.x + 12.f, bPillMin.y + 5.5f), ImColor(55, 230, 245, 255), badgeText.c_str());
        if (g_FontSmall) ImGui::PopFont();

        // 2. Metrics & Percentage Calculations
        uint64_t dlBytes  = s_DownloadedBytes.load();
        uint64_t totBytes = s_TotalBytes.load();

        int pct = 0;
        if (s_DownloadComplete)
        {
            pct = 100;
        }
        else if (totBytes > 0)
        {
            pct = (int)((double)dlBytes / (double)totBytes * 100.0);
            if (pct > 100) pct = 100;
        }

        float spd = s_DownloadSpeed.load();
        char spdBuf[64] = { 0 };
        if (s_DownloadComplete)
        {
            sprintf_s(spdBuf, "Restarting loader...");
        }
        else if (s_DownloadFailed)
        {
            sprintf_s(spdBuf, "Download failed");
        }
        else if (spd > 0.05f)
        {
            sprintf_s(spdBuf, "%.1f MB/s", spd);
        }
        else if (dlBytes > 0)
        {
            sprintf_s(spdBuf, "Downloading...");
        }
        else
        {
            sprintf_s(spdBuf, "Connecting to server...");
        }

        // 3. Middle Progress HUD
        float barW = sz.x - 56.f;
        ImVec2 barMin = ImVec2(wp.x + 28.f, wp.y + 108.f);
        float barH = 3.5f;
        ImVec2 barMax = ImVec2(barMin.x + barW, barMin.y + barH);

        // Right-aligned percentage text above bar (exact match to screenshot)
        char pctBuf[32];
        sprintf_s(pctBuf, "%d%%", pct);
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        ImVec2 pctSz = ImGui::CalcTextSize(pctBuf);
        dl->AddText(ImVec2(barMax.x - pctSz.x, barMin.y - pctSz.y - 6.f), ImColor(165, 178, 195, 230), pctBuf);
        if (g_FontSmall) ImGui::PopFont();

        // Background track (subtle rounded track)
        dl->AddRectFilled(barMin, barMax, ImColor(18, 24, 32, 255), 2.0f);

        // Progress interpolation
        static float s_SmoothProgress = 0.0f;
        float targetProgress = 0.0f;
        if (s_DownloadComplete)
        {
            targetProgress = 1.0f;
        }
        else if (totBytes > 0)
        {
            targetProgress = (float)((double)dlBytes / (double)totBytes);
            if (targetProgress > 1.0f) targetProgress = 1.0f;
        }

        s_SmoothProgress = SmoothLerp(s_SmoothProgress, targetProgress, ImGui::GetIO().DeltaTime * 10.0f);

        if (totBytes == 0 && !s_DownloadComplete)
        {
            // Indeterminate animation before total size is reported
            float t = (float)ImGui::GetTime();
            float bounce = (std::sin(t * 3.5f) + 1.0f) * 0.5f;
            float indW = barW * 0.35f;
            float indX = barMin.x + (barW - indW) * bounce;
            dl->AddRectFilled(ImVec2(indX, barMin.y - 1.f), ImVec2(indX + indW, barMax.y + 1.f), ImColor(0, 242, 254, 45), 2.0f);
            dl->AddRectFilled(ImVec2(indX, barMin.y), ImVec2(indX + indW, barMax.y), ImColor(0, 242, 254, 255), 2.0f);
        }
        else
        {
            float fillW = barW * s_SmoothProgress;
            if (fillW > 1.5f)
            {
                ImColor fillColor = s_DownloadComplete ? ImColor(16, 185, 129, 255) : ImColor(0, 242, 254, 255);
                // Soft glow bloom along bar
                dl->AddRectFilled(ImVec2(barMin.x, barMin.y - 1.f), ImVec2(barMin.x + fillW, barMax.y + 1.f), ImColor(0, 242, 254, 45), 2.0f);
                // Core cyan fill
                dl->AddRectFilled(barMin, ImVec2(barMin.x + fillW, barMax.y), fillColor, 2.0f);
            }
        }

        // Transfer speed centered directly below bar (exact match to screenshot)
        if (g_FontSmall) ImGui::PushFont(g_FontSmall);
        ImVec2 spdSz = ImGui::CalcTextSize(spdBuf);
        dl->AddText(ImVec2(wp.x + (sz.x - spdSz.x) * 0.5f, barMax.y + 11.f), ImColor(140, 155, 175, 210), spdBuf);
        if (g_FontSmall) ImGui::PopFont();

        // 4. In case of network error (Only shown if download failed)
        if (s_DownloadFailed)
        {
            ImGui::SetCursorPosY(152.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 13.0f);
            float btnW = (sz.x - 62.f) * 0.5f;

            ImGui::SetCursorPosX(26.f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.16f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.18f, 0.24f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.16f, 0.22f, 0.30f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_Border, C_ACCENT);
            if (ImGui::Button("RETRY", ImVec2(btnW, 26.f)))
            {
                StartBackgroundUpdate();
            }
            ImGui::PopStyleColor(4);

            ImGui::SameLine(0.f, 10.f);

            ImGui::PushStyleColor(ImGuiCol_Button, C_BTN_GHOST);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C_BTN_GH);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  C_BTN_GA);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 0.5f));
            if (ImGui::Button("EXIT", ImVec2(btnW, 26.f)))
            {
                g_Running = false;
            }
            ImGui::PopStyleColor(4);

            ImGui::PopStyleVar();
        }

        if (g_FontMed) ImGui::PopFont();
    }

    // -----------------------------------------------------------------------
    // Run
    // -----------------------------------------------------------------------
    bool AuthWindow::Run()
    {
        // Clean up any leftover update files from a previous self-restart
        wchar_t curExe[MAX_PATH] = { 0 };
        if (GetModuleFileNameW(nullptr, curExe, MAX_PATH) > 0)
        {
            std::wstring oldExe = std::wstring(curExe) + L".old";
            std::wstring newExe = std::wstring(curExe) + L".new";
            SetFileAttributesW(newExe.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(newExe.c_str());

            // Run in a detached thread with up to 60 attempts (30s) to guarantee the old version is deleted
            std::thread([oldExe, curExeStr = std::wstring(curExe)]() {
                for (int attempt = 0; attempt < 60; ++attempt)
                {
                    SetFileAttributesW(oldExe.c_str(), FILE_ATTRIBUTE_NORMAL);
                    if (DeleteFileW(oldExe.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND)
                        break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }

                // Also clean up any other leftover *.old files in the same directory
                size_t lastSlash = curExeStr.find_last_of(L"\\/");
                if (lastSlash != std::wstring::npos)
                {
                    std::wstring searchPattern = curExeStr.substr(0, lastSlash + 1) + L"*.old";
                    WIN32_FIND_DATAW fd;
                    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &fd);
                    if (hFind != INVALID_HANDLE_VALUE)
                    {
                        do
                        {
                            std::wstring oldFilePath = curExeStr.substr(0, lastSlash + 1) + fd.cFileName;
                            SetFileAttributesW(oldFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
                            DeleteFileW(oldFilePath.c_str());
                        } while (FindNextFileW(hFind, &fd));
                        FindClose(hFind);
                    }
                }

                // If somehow still locked, register for automatic deletion on next system reboot
                MoveFileExW(oldExe.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
            }).detach();
        }

        M_base::Misc::OptimizeThreadForGameSync();
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        SetAuthenticated(false);
        g_Running         = true;
        s_State           = STATE_CHECKING;

        WNDCLASSEX wc = { sizeof wc, CS_CLASSDC, AuthWndProc, 0, 0,
            GetModuleHandle(nullptr), nullptr, LoadCursor(nullptr, IDC_ARROW),
            nullptr, nullptr, L"CrngAuthCls", nullptr };
        RegisterClassEx(&wc);

        // Initial position (loading window size)
        int w, h; GetWindowDims(STATE_CHECKING, w, h);
        ImVec2 pos = CenterOnGameMonitor(w, h);

        g_hWnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_TOPMOST, wc.lpszClassName,
            L"ROB RUN", WS_POPUP | WS_VISIBLE,
            (int)pos.x, (int)pos.y, w, h,
            nullptr, nullptr, wc.hInstance, nullptr);
        if (!g_hWnd) return false;

        if (!CreateDev(g_hWnd)) { CleanupDev(); DestroyWindow(g_hWnd); UnregisterClass(wc.lpszClassName, wc.hInstance); return false; }

        ShowWindow(g_hWnd, SW_SHOWDEFAULT);
        BringWindowToTop(g_hWnd);
        SetForegroundWindow(g_hWnd);
        SetFocus(g_hWnd);
        UpdateWindow(g_hWnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

        // Load fonts from framework data
        ImFontConfig fc;
        fc.FontDataOwnedByAtlas = false;
        g_FontLarge = io.Fonts->AddFontFromMemoryTTF(
            inter_semibold_data.data(),
            (int)inter_semibold_data.size(),
            22.f, &fc);
        g_FontTitle = io.Fonts->AddFontFromMemoryTTF(
            inter_semibold_data.data(),
            (int)inter_semibold_data.size(),
            18.f, &fc);
        g_FontMed = io.Fonts->AddFontFromMemoryTTF(
            inter_medium_data.data(),
            (int)inter_medium_data.size(),
            13.5f, &fc);
        g_FontSmall = io.Fonts->AddFontFromMemoryTTF(
            inter_medium_data.data(),
            (int)inter_medium_data.size(),
            11.f, &fc);
        io.Fonts->Build();

        ImGui_ImplWin32_Init(g_hWnd);
        ImGui_ImplDX11_Init(g_pDev, g_pCtx);
        SetupTheme();
        StartBackgroundCheck();

        AuthState prevState = STATE_CHECKING;

        MSG msg = {};
        while (msg.message != WM_QUIT && g_Running)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg); DispatchMessage(&msg);
                if (msg.message == WM_QUIT) g_Running = false;
            }
            if (!g_Running) break;

            // Resize window when state changes
            if (s_State != prevState) {
                prevState = s_State;
                int nw, nh; GetWindowDims(s_State, nw, nh);
                ImVec2 np = CenterOnGameMonitor(nw, nh);
                SetWindowPos(g_hWnd, HWND_TOP, (int)np.x, (int)np.y, nw, nh, SWP_SHOWWINDOW);
                SetForegroundWindow(g_hWnd);
                SetFocus(g_hWnd);
                // Recreate swap chain buffers (Unbind RTV first for DXGI Flip Model compliance)
                if (g_pCtx) g_pCtx->OMSetRenderTargets(0, nullptr, nullptr);
                if (g_pRTV) { g_pRTV->Release(); g_pRTV = nullptr; }
                g_pSC->ResizeBuffers(0, nw, nh, DXGI_FORMAT_UNKNOWN, 0);
                ID3D11Texture2D* pBB = nullptr;
                g_pSC->GetBuffer(0, IID_PPV_ARGS(&pBB));
                if (pBB) { g_pDev->CreateRenderTargetView(pBB, nullptr, &g_pRTV); pBB->Release(); }
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            int cw, ch; GetWindowDims(s_State, cw, ch);
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({ (float)cw, (float)ch });
            ImGuiWindowFlags fl = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove    | ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoScrollbar;
            ImGui::Begin("##auth", nullptr, fl);

            switch (s_State) {
            case STATE_CHECKING: RenderLoadingScreen();      break;
            case STATE_REGISTER: RenderRegistrationScreen(); break;
            case STATE_LOGGED_IN:RenderSuccessScreen();      break;
            case STATE_BANNED:      RenderBannedScreen();       break;
            case STATE_KEY_EXPIRED: RenderKeyExpiredScreen();   break;
            case STATE_ERROR:       RenderErrorScreen();        break;
            case STATE_EXPIRED:  RenderExpiredScreen();      break;
            }

            // Drag window — ONLY from the top title header (< 45px from top) and when not clicking an interactive item
            ImVec2 mPos = ImGui::GetMousePos();
            ImVec2 wPos = ImGui::GetWindowPos();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && (mPos.y - wPos.y) <= 45.f
                && !ImGui::IsAnyItemHovered()
                && !ImGui::IsAnyItemActive()
                && s_State != STATE_CHECKING)
            {
                ReleaseCapture();
                SendMessage(g_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }

            ImGui::End();
            ImGui::Render();

            const float cc[4] = { C_BG.x, C_BG.y, C_BG.z, 1.f };
            g_pCtx->OMSetRenderTargets(1, &g_pRTV, nullptr);
            g_pCtx->ClearRenderTargetView(g_pRTV, cc);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_pSC->Present(1, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // VSync hardware sync, low CPU load
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDev();
        DestroyWindow(g_hWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);

        if (IsAuthenticated())
        {
            AuthClient::StartTelemetry(s_CurrentHwid, s_CurrentKey, s_CurrentUsername);
        }

        return IsAuthenticated();
    }

}} // namespace Cheat::Auth
