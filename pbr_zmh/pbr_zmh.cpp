//--------------------------------------------------------------------------------------
// File: EmptyProject11.cpp
//
// Empty starting point for new Direct3D 11 Win32 desktop applications
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "DXUTRes.h"
#include "resource.h"
#include "DDSTextureLoader.h"

#pragma warning( disable : 4100 )

using namespace DirectX;


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CModelViewerCamera                  g_Camera;               // A model viewing camera
CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                     g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                         g_HUD;                  // manages the 3D UI
CDXUTDialog                         g_SampleUI;             // dialog for sample specific controls
CDXUTTextHelper*                    g_pTxtHelper = nullptr;


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_STATIC                 -1
#define IDC_CHANGEDEVICE            1


//
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);
void InitApp();


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
	HRESULT hr;

	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN(g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	V_RETURN(g_D3DSettingsDlg.OnD3D11CreateDevice(pd3dDevice));
	g_pTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15);

	// Setup the camera's view parameters
	static const XMVECTORF32 s_vEyeStart = { 0.f, 0.f, -5.f, 0.f };
	static const XMVECTORF32 s_vAtStart = { 0.f, 0.f, 0.f, 0.f };
	g_Camera.SetViewParams(s_vEyeStart, s_vAtStart);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	HRESULT hr = S_OK;

	V_RETURN(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	V_RETURN(g_D3DSettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(53.4f * (XM_PI / 180.0f), fAspectRatio, 20.0f, 30000.0f);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_Camera.SetButtonMasks(0, MOUSE_WHEEL, MOUSE_RIGHT_BUTTON | MOUSE_LEFT_BUTTON);

	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	g_HUD.SetSize(170, 170);
	g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300);
	g_SampleUI.SetSize(170, 300);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	// Update the camera's position based on user input 
	g_Camera.FrameMove(fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                  double fTime, float fElapsedTime, void* pUserContext )
{
	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if (g_D3DSettingsDlg.IsActive())
	{
		g_D3DSettingsDlg.OnRender(fElapsedTime);
		return;
	}

	auto pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::MidnightBlue);
	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	//XMMATRIX mProj = g_Camera.GetProjMatrix();
	//XMMATRIX mView = g_Camera.GetViewMatrix();

	/*render 3d here*/

	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
	g_HUD.OnRender(fElapsedTime);
	g_SampleUI.OnRender(fElapsedTime);
	//RenderText();
	DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
	g_D3DSettingsDlg.OnD3D11DestroyDevice();
	SAFE_DELETE(g_pTxtHelper);
	DXUTGetGlobalResourceCache().OnDestroyDevice();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass messages to settings dialog if its active
	if (g_D3DSettingsDlg.IsActive())
	{
		g_D3DSettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;
	*pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass all remaining windows messages to camera so it can respond to user input
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                       bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                       int xPos, int yPos, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#ifdef _DEBUG
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device
    // that is available on the system depending on which D3D callbacks are set below

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse( OnMouse );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

    // Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    // Perform any application-level initialization here
	InitApp();
    DXUTInit( true, true, nullptr ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"PBR" );

    // Only require 10-level hardware or later
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 800, 600 );
    DXUTMainLoop(); // Enter into the DXUT ren  der loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}


void InitApp()
{
	g_D3DSettingsDlg.Init(&g_DialogResourceManager);
	g_HUD.Init(&g_DialogResourceManager);
	g_SampleUI.Init(&g_DialogResourceManager);
	g_SampleUI.SetCallback(OnGUIEvent);

	g_HUD.SetCallback(OnGUIEvent);
	int iY = 10;
	g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY, 170, 23, VK_F2);
}


void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	switch (nControlID)
	{
		case IDC_CHANGEDEVICE:
				g_D3DSettingsDlg.SetActive(!g_D3DSettingsDlg.IsActive()); break;
	}
}


