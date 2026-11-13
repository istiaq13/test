#include "Overlay.hpp"
#include "kaanlar.h"
#include <Main/Options.hpp>

struct WndRECT : public RECT
{
	int Width() { return right - left; }
	int Height() { return bottom - top; }
};

static inline std::function<void(HWND, UINT, WPARAM, LPARAM)> pWindowProc;

bool bSettuped = false;
bool bInitialized = false;

bool bDeviceInitialized;
bool bRenderTargetInitialized;

HWND hWindow;
WNDCLASSEX WindowClass;
HWND hTargetWindow;
WndRECT wTargetWindowRect;
DWORD sTargetPid;

ID3D11Device* ID3dDevice;
ID3D11DeviceContext* ID3dDeviceContext;
IDXGISwapChain* ID3dSwapChain;
ID3D11RenderTargetView* ID3dRenderTargetView;

void CreateDeviceD3D()
{
	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	ZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));
	SwapChainDesc.BufferDesc.Width = 0;
	SwapChainDesc.BufferDesc.Height = 0;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.OutputWindow = hWindow;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	D3D_FEATURE_LEVEL FeatureLevel;
	const D3D_FEATURE_LEVEL FeatureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, FeatureLevelArray, 2, D3D11_SDK_VERSION, &SwapChainDesc, &ID3dSwapChain, &ID3dDevice, &FeatureLevel, &ID3dDeviceContext) != S_OK)
	{

		return;
	}

	bDeviceInitialized = true;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (pWindowProc)
		pWindowProc(hWnd, uMsg, wParam, lParam);

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


namespace M_base
{
	void Overlay::Setup(DWORD TargetPid)
	{
		sTargetPid = TargetPid;

		hTargetWindow = mem::GetWindowHandleByPID(TargetPid);
		if (hTargetWindow)
		{
			SafeCall(GetClientRect)(hTargetWindow, &wTargetWindowRect);
			SafeCall(MapWindowPoints)(hTargetWindow, nullptr, reinterpret_cast<LPPOINT>(&wTargetWindowRect), 2);
		}
		if (!hTargetWindow || wTargetWindowRect.Width() < 100 || wTargetWindowRect.Height() < 100)
		{
			// Create a fullscreen overlay if no valid target window rect is found
			wTargetWindowRect.left = 0;
			wTargetWindowRect.top = 0;
			wTargetWindowRect.right = GetSystemMetrics(SM_CXSCREEN);
			wTargetWindowRect.bottom = GetSystemMetrics(SM_CYSCREEN);
		}

		bSettuped = true;
	}

	void Overlay::Initialize()
	{
		if (!bSettuped)
		{
			return;
		}

		WindowClass.cbSize = sizeof(WindowClass);
		WindowClass.style = CS_HREDRAW | CS_VREDRAW;
		WindowClass.lpfnWndProc = WindowProc;
		WindowClass.cbClsExtra = 0;
		WindowClass.cbWndExtra = 0;
		WindowClass.hInstance = GetModuleHandle(NULL);
		WindowClass.hIcon = NULL;
		WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		WindowClass.hbrBackground = HBRUSH(RGB(0, 0, 0));
		WindowClass.lpszMenuName = NULL;
		WindowClass.lpszClassName = xorstr_(L"Window");
		WindowClass.hIconSm = NULL;

		ATOM Class = SafeCall(RegisterClassEx)(&WindowClass);

		if (!Class)
		{

			return;
		}

		hWindow = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED, WindowClass.lpszClassName, WindowClass.lpszMenuName, WS_POPUP | WS_VISIBLE, wTargetWindowRect.left, wTargetWindowRect.top, wTargetWindowRect.Width(), wTargetWindowRect.Height(), NULL, NULL, GetModuleHandle(NULL), NULL);

		if (!hWindow)
		{

			return;
		}

		MARGINS Margins = { wTargetWindowRect.left, wTargetWindowRect.top, wTargetWindowRect.Width(), wTargetWindowRect.Height() };
		DwmExtendFrameIntoClientArea(hWindow, &Margins);

		SafeCall(SetLayeredWindowAttributes)(hWindow, RGB(0, 0, 0), 255, LWA_ALPHA);

		SafeCall(ShowWindow)(hWindow, SW_SHOWDEFAULT);
		SafeCall(UpdateWindow)(hWindow);

		if (g_Options.General.StreamProof)
		{
			if (!SetWindowDisplayAffinity(hWindow, 0x00000011))
			{
				SetWindowDisplayAffinity(hWindow, 0x00000001);
			}
		}

		bInitialized = true;

		dxInitialize();
	}

	void Overlay::ShutDown()
	{
		SafeCall(DestroyWindow)(hWindow);
		SafeCall(UnregisterClass)(WindowClass.lpszClassName, WindowClass.hInstance);

		bInitialized = false; bSettuped = false;
	}

	void Overlay::UpdateWindowPos()
	{
		if (hTargetWindow)
		{
			WndRECT TargetWindowRect;
			GetClientRect(hTargetWindow, &TargetWindowRect);
			MapWindowPoints(hTargetWindow, nullptr, reinterpret_cast<LPPOINT>(&TargetWindowRect), 2);
			static RECT s_LastTargetRect = { 0, 0, 0, 0 };
			if (TargetWindowRect.left != s_LastTargetRect.left ||
				TargetWindowRect.top != s_LastTargetRect.top ||
				TargetWindowRect.right != s_LastTargetRect.right ||
				TargetWindowRect.bottom != s_LastTargetRect.bottom)
			{
				MoveWindow(hWindow, TargetWindowRect.left, TargetWindowRect.top, TargetWindowRect.Width(), TargetWindowRect.Height(), false);
				s_LastTargetRect = TargetWindowRect;
			}
		}
	}

	void Overlay::SetupWindowProcHook(std::function<void(HWND, UINT, WPARAM, LPARAM)> Funtion)
	{
		pWindowProc = Funtion;
	}

	void Overlay::dxInitialize()
	{
		CreateDeviceD3D();
		if (bDeviceInitialized)
		{
			dxCreateRenderTarget();
		}
	}

	void Overlay::dxRefresh()
	{
		ID3dDeviceContext->OMSetRenderTargets(1, &ID3dRenderTargetView, nullptr);
		static float TransparentColor[4] = { 0, 0, 0, 0 };
		ID3dDeviceContext->ClearRenderTargetView(ID3dRenderTargetView, TransparentColor);
	}

	void Overlay::dxShutDown()
	{
		dxCleanupRenderTarget();
		dxCleanupDeviceD3D();
	}

	void Overlay::dxCreateRenderTarget()
	{
		ID3D11Texture2D* pBackBuffer;
		ID3dSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		ID3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &ID3dRenderTargetView);
		pBackBuffer->Release();

		bRenderTargetInitialized = true;
	}

	void Overlay::dxCleanupRenderTarget()
	{
		if (ID3dRenderTargetView) { ID3dRenderTargetView->Release(); ID3dRenderTargetView = NULL; }

		bRenderTargetInitialized = false;
	}

	void Overlay::dxCleanupDeviceD3D()
	{
		if (ID3dSwapChain) { ID3dSwapChain->Release(); ID3dSwapChain = NULL; }
		if (ID3dDeviceContext) { ID3dDeviceContext->Release(); ID3dDeviceContext = NULL; }
		if (ID3dDevice) { ID3dDevice->Release(); ID3dDevice = NULL; }

		bDeviceInitialized = false;
	}

	bool Overlay::IsSettuped() { return bSettuped; }
	bool Overlay::IsInitialized() { return bInitialized; }
	HWND Overlay::GetOverlayWindow() { return hWindow; }
	HWND Overlay::GetTargetWindow() { return hTargetWindow; }

	ID3D11Device* Overlay::dxGetDevice() { return ID3dDevice; }
	ID3D11DeviceContext* Overlay::dxGetDeviceContext() { return ID3dDeviceContext; }
	IDXGISwapChain* Overlay::dxGetSwapChain() { return ID3dSwapChain; }
	ID3D11RenderTargetView* Overlay::dxGetRenderTarget() { return ID3dRenderTargetView; }
}
