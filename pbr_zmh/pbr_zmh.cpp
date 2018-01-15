#include "Precompiled.h"
#include "resource.h"

#pragma warning( disable : 4100 )

using namespace DirectX;


//
struct GlobalParams
{
	XMMATRIX ViewProjMatrix;
	XMVECTOR ViewPos;
	XMVECTOR LightDir;
};


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CModelViewerCamera                  g_Camera;               // A model viewing camera
CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                     g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                         g_HUD;                  // manages the 3D UI
CDXUTDialog                         g_SampleUI;             // dialog for sample specific controls
CDXUTTextHelper*                    g_pTxtHelper = nullptr;

ID3D11Buffer*						g_globalParamsBuf = nullptr;
ID3D11DepthStencilState*			g_depthStencilState = nullptr;
ID3D11RasterizerState*				g_rasterizerState = nullptr;
SphereRenderer						g_sphereRenderer;

int g_lightDirVert = 45;
int g_lightDirHor = 130;
float g_metalness = 1.0f;
float g_roughness = 0.5f;


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
enum IDC
{
	IDC_CHANGEDEVICE = 1,
	IDC_LIGHTVERT_STATIC,
	IDC_LIGHTVERT,
	IDC_LIGHTHOR_STATIC,
	IDC_LIGHTHOR,
	IDC_METALNESS_STATIC,
	IDC_METALNESS,
	IDC_ROUGHNESS_STATIC,
	IDC_ROUGHNESS,
};

//
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);
void InitApp();


//
float ToRad( float deg )
{
	return deg * XM_PI / 180.0f;
}


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

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof( GlobalParams );
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &g_globalParamsBuf ) );
	DXUT_SetDebugName( g_globalParamsBuf, "GlobalParams" );

	// Setup the camera's view parameters
	static const XMVECTORF32 s_vEyeStart = { 0.f, 0.f, -5.f, 0.f };
	static const XMVECTORF32 s_vAtStart = { 0.f, 0.f, 0.f, 0.f };
	g_Camera.SetViewParams(s_vEyeStart, s_vAtStart);

	g_sphereRenderer.OnD3D11CreateDevice( pd3dDevice );

	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
	rasterDesc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterDesc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.ScissorEnable = FALSE;
	rasterDesc.MultisampleEnable = FALSE;
	rasterDesc.AntialiasedLineEnable = FALSE;
	pd3dDevice->CreateRasterizerState( &rasterDesc, &g_rasterizerState );

	D3D11_DEPTH_STENCIL_DESC depthDesc;
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDesc.StencilEnable = FALSE;
	depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS };
	depthDesc.FrontFace = defaultStencilOp;
	depthDesc.BackFace = defaultStencilOp;
	pd3dDevice->CreateDepthStencilState( &depthDesc, &g_depthStencilState );

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
	g_Camera.SetProjParams(53.4f * (XM_PI / 180.0f), fAspectRatio, 1.0f, 30000.0f);
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

	pd3dImmediateContext->RSSetState( g_rasterizerState );
	pd3dImmediateContext->OMSetDepthStencilState( g_depthStencilState, 0 );

	// update global params
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubres;
		pd3dImmediateContext->Map( g_globalParamsBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );

		GlobalParams* globalParams = ( GlobalParams* )mappedSubres.pData;
		globalParams->ViewProjMatrix = XMMatrixMultiply( g_Camera.GetViewMatrix(), g_Camera.GetProjMatrix() );
		globalParams->ViewPos = g_Camera.GetEyePt();
		float lightDirVert = ToRad( ( float )g_lightDirVert );
		float lightDirHor = ToRad( ( float )g_lightDirHor );
		globalParams->LightDir = XMVectorSet( sin( lightDirVert ) * sin( lightDirHor ), cos( lightDirVert ), sin( lightDirVert ) * cos( lightDirHor ), 0.0f );

		pd3dImmediateContext->Unmap( g_globalParamsBuf, 0 );

		// apply
		pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &g_globalParamsBuf );
		pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &g_globalParamsBuf );
	}


	//
	g_sphereRenderer.Render( XMMatrixIdentity(), g_metalness, g_roughness, pd3dImmediateContext );

	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
	g_HUD.OnRender(fElapsedTime);
	g_SampleUI.OnRender(fElapsedTime);
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos( 2, 0 );
	g_pTxtHelper->SetForegroundColor( Colors::Yellow );
	g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
	g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
	g_pTxtHelper->End();
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
	SAFE_RELEASE( g_globalParamsBuf );
	SAFE_RELEASE( g_rasterizerState );
	SAFE_RELEASE( g_depthStencilState );

	g_sphereRenderer.OnD3D11DestroyDevice();
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

	iY += 50;
	WCHAR str[MAX_PATH];
	swprintf_s(str, MAX_PATH, L"Light vert: %d", g_lightDirVert);
	g_HUD.AddStatic(IDC_LIGHTVERT_STATIC, str, 25, iY += 24, 135, 22);
	g_HUD.AddSlider(IDC_LIGHTVERT, 15, iY += 24, 135, 22, 0, 180, g_lightDirVert );

	swprintf_s(str, MAX_PATH, L"Light hor: %d", g_lightDirHor);
	g_HUD.AddStatic(IDC_LIGHTHOR_STATIC, str, 25, iY += 24, 135, 22);
	g_HUD.AddSlider(IDC_LIGHTHOR, 15, iY += 24, 135, 22, 0, 180, g_lightDirHor );

	swprintf_s( str, MAX_PATH, L"Metalness: %1.2f", g_metalness );
	g_HUD.AddStatic( IDC_METALNESS_STATIC, str, 25, iY += 24, 135, 22 );
	g_HUD.AddSlider( IDC_METALNESS, 15, iY += 24, 135, 22, 0, 100, ( int )( g_metalness * 100.0f ) );

	swprintf_s( str, MAX_PATH, L"Roughness: %1.2f", g_roughness );
	g_HUD.AddStatic( IDC_ROUGHNESS_STATIC, str, 25, iY += 24, 135, 22 );
	g_HUD.AddSlider( IDC_ROUGHNESS, 15, iY += 24, 135, 22, 0, 100, ( int )( g_roughness * 100.0f ) );
}


void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	WCHAR str[MAX_PATH];

	switch (nControlID)
	{
		case IDC_CHANGEDEVICE:
			g_D3DSettingsDlg.SetActive(!g_D3DSettingsDlg.IsActive()); 
			break;
		case IDC_LIGHTVERT:
		{
			g_lightDirVert = g_HUD.GetSlider(IDC_LIGHTVERT)->GetValue();
			swprintf_s(str, MAX_PATH, L"Light vert: %d", g_lightDirVert);
			g_HUD.GetStatic(IDC_LIGHTVERT_STATIC)->SetText(str);
			break;
		}
		case IDC_LIGHTHOR:
		{
			g_lightDirHor = g_HUD.GetSlider(IDC_LIGHTHOR)->GetValue();
			swprintf_s(str, MAX_PATH, L"Light hor: %d", g_lightDirHor);
			g_HUD.GetStatic(IDC_LIGHTHOR_STATIC)->SetText(str);
			break;
		}
		case IDC_METALNESS:
		{
			g_metalness = g_HUD.GetSlider( IDC_METALNESS )->GetValue() / 100.0f;
			swprintf_s( str, MAX_PATH, L"Metalness: %1.2f", g_metalness );
			g_HUD.GetStatic( IDC_METALNESS_STATIC )->SetText( str );
			break;
		}
		case IDC_ROUGHNESS:
		{
			g_roughness = g_HUD.GetSlider( IDC_ROUGHNESS )->GetValue() / 100.0f;
			swprintf_s( str, MAX_PATH, L"Roughness: %1.2f", g_roughness );
			g_HUD.GetStatic( IDC_ROUGHNESS_STATIC )->SetText( str );
			break;
		}
	}
}


