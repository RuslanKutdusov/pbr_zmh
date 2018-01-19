#include "Precompiled.h"
#include "resource.h"
#include "shaders/global_registers.h"

#pragma warning( disable : 4100 )

using namespace DirectX;


//
struct GlobalParams
{
	XMMATRIX ViewProjMatrix;
	XMVECTOR ViewPos;
	XMVECTOR LightDir;
	UINT FrameIdx;
	UINT padding[ 3 ];
};


enum SCENE_TYPE
{
	SCENE_ONE_SPHERE = 0,
	SCENE_MULTIPLE_SPHERES,
	//SCENE_SPONZA

	SCENE_TYPE_COUNT
};


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
enum IDC
{
	IDC_CHANGEDEVICE = 1,
	IDC_RELOAD_SHADERS,
	IDC_LIGHTVERT_STATIC,
	IDC_LIGHTVERT,
	IDC_LIGHTHOR_STATIC,
	IDC_LIGHTHOR,
	IDC_METALNESS_STATIC,
	IDC_METALNESS,
	IDC_ROUGHNESS_STATIC,
	IDC_ROUGHNESS,
	IDC_DIRECT_LIGHT,
	IDC_INDIRECT_LIGHT,
	IDC_EXPOSURE_STATIC,
	IDC_EXPOSURE,
	IDC_DRAW_SKY,
	IDC_SELECT_SKY_TEXTURE,
	IDC_SCENE_TYPE,
};


const int HUD_WIDTH = 250;


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CModelViewerCamera                  g_modelViewerCamera;     // A model viewing camera
CFirstPersonCamera					g_firstPersonCamera;
CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                     g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                         g_HUD;                  // manages the 3D UI
CDXUTTextHelper*                    g_pTxtHelper = nullptr;

ID3D11Buffer*						g_globalParamsBuf = nullptr;
ID3D11DepthStencilState*			g_depthStencilState = nullptr;
ID3D11RasterizerState*				g_rasterizerState = nullptr;
ID3D11SamplerState*					g_linearWrapSamplerState = nullptr;
SphereRenderer						g_sphereRenderer;
SkyRenderer							g_skyRenderer;
PostProcess							g_postProcess;

// hdr stuff
ID3D11Texture2D*					g_hdrBackbufferTexture;
ID3D11RenderTargetView*				g_hdrBackbufferRTV;
ID3D11ShaderResourceView*			g_hdrBackbufferSRV;

ID3D11VertexShader* g_lineVs = nullptr;
ID3D11VertexShader* g_planeVs = nullptr;
ID3D11PixelShader* g_linePs = nullptr;
ID3D11Buffer*		g_lineParamsBuf = nullptr;
struct LineParams
{
	float Metalness;
	float Roughness;
	UINT NumSamples;
	UINT padding;
};

int g_lightDirVert = 45;
int g_lightDirHor = 130;
float g_metalness = 1.0f;
float g_roughness = 0.5f;
bool g_directLight = true;
bool g_indirectLight = true;
float g_exposure = 1.0f;
bool g_drawSky = true;
SCENE_TYPE g_sceneType = SCENE_ONE_SPHERE;
UINT g_frameIdx = 0;

const wchar_t* g_skyTextureFiles[] = {
	L"HDRs\\galileo_cross.dds",
	L"HDRs\\grace_cross.dds",
	L"HDRs\\rnl_cross.dds",
	L"HDRs\\stpeters_cross.dds",
	L"HDRs\\uffizi_cross.dds",
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
	XMVECTORF32 s_vEyeStart = { 0.f, 0.f, -5.f, 0.f };
	XMVECTORF32 s_vAtStart = { 0.f, 0.f, 0.f, 0.f };
	g_modelViewerCamera.SetViewParams( s_vEyeStart, s_vAtStart );
	s_vEyeStart = { 0.f, 5.f, -15.f, 0.f };
	g_firstPersonCamera.SetViewParams( s_vEyeStart, s_vAtStart );

	V_RETURN( g_sphereRenderer.OnD3D11CreateDevice( pd3dDevice ) );
	V_RETURN( g_skyRenderer.OnD3D11CreateDevice( pd3dDevice ) );
	V_RETURN( g_skyRenderer.LoadSkyTexture( pd3dDevice, g_skyTextureFiles[ 0 ] ) );
	V_RETURN( g_postProcess.OnD3D11CreateDevice( pd3dDevice ) );

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

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[ 0 ] = samplerDesc.BorderColor[ 1 ] = samplerDesc.BorderColor[ 2 ] = samplerDesc.BorderColor[ 3 ] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN( pd3dDevice->CreateSamplerState( &samplerDesc, &g_linearWrapSamplerState ) );

	//
	ID3DBlob* blob = nullptr;
	V_RETURN( CompileShader( L"shaders\\line.hlsl", nullptr, "vs_line", SHADER_VERTEX, &blob ) );
	V_RETURN( pd3dDevice->CreateVertexShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_lineVs ) );
	DXUT_SetDebugName( g_lineVs, "LineVS" );
	SAFE_RELEASE( blob );

	V_RETURN( CompileShader( L"shaders\\line.hlsl", nullptr, "vs_plane", SHADER_VERTEX, &blob ) );
	V_RETURN( pd3dDevice->CreateVertexShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_planeVs ) );
	DXUT_SetDebugName( g_planeVs, "PlaneVS" );
	SAFE_RELEASE( blob );

	V_RETURN( CompileShader( L"shaders\\line.hlsl", nullptr, "ps_main", SHADER_PIXEL, &blob ) );
	V_RETURN( pd3dDevice->CreatePixelShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_linePs ) );
	DXUT_SetDebugName( g_linePs, "LinePS" );
	SAFE_RELEASE( blob );

	Desc.ByteWidth = sizeof( LineParams );
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &g_lineParamsBuf ) );
	DXUT_SetDebugName( g_lineParamsBuf, "LineParams" );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	HRESULT hr = S_OK;

	{

		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory( &texDesc, sizeof( D3D11_TEXTURE2D_DESC ) );
		texDesc.ArraySize = 1;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.Width = pBackBufferSurfaceDesc->Width;
		texDesc.Height = pBackBufferSurfaceDesc->Height;
		texDesc.MipLevels = 1;
		texDesc.SampleDesc.Count = 1;
		V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, nullptr, &g_hdrBackbufferTexture ) );
		DXUT_SetDebugName( g_hdrBackbufferTexture, "HdrBackbufferTexture" );

		// Create the render target view
		D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
		rtDesc.Format = texDesc.Format;
		rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		V_RETURN( pd3dDevice->CreateRenderTargetView( g_hdrBackbufferTexture, &rtDesc, &g_hdrBackbufferRTV ) );
		DXUT_SetDebugName( g_hdrBackbufferRTV, "HdrBackbufferRTV" );

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		V_RETURN( pd3dDevice->CreateShaderResourceView( g_hdrBackbufferTexture, &srvDesc, &g_hdrBackbufferSRV ) );
	}

	V_RETURN(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	V_RETURN(g_D3DSettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_modelViewerCamera.SetProjParams( 53.4f * ( XM_PI / 180.0f ), fAspectRatio, 0.1f, 3000.0f );
	g_modelViewerCamera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
	g_modelViewerCamera.SetButtonMasks( 0, MOUSE_WHEEL, MOUSE_RIGHT_BUTTON | MOUSE_LEFT_BUTTON );
	g_firstPersonCamera.SetProjParams( 53.4f * ( XM_PI / 180.0f ), fAspectRatio, 0.1f, 3000.0f );

	g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - HUD_WIDTH - 10, 0 );
	g_HUD.SetSize( HUD_WIDTH, 170 );

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	// Update the camera's position based on user input 
	if( g_sceneType == SCENE_ONE_SPHERE )
		g_modelViewerCamera.FrameMove( fElapsedTime );
	else
		g_firstPersonCamera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                  double fTime, float fElapsedTime, void* pUserContext )
{
	g_frameIdx++;

	auto pRTV = DXUTGetD3D11RenderTargetView();
	auto pDSV = DXUTGetD3D11DepthStencilView();

	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if (g_D3DSettingsDlg.IsActive())
	{
		g_D3DSettingsDlg.OnRender(fElapsedTime);
		return;
	}

	pd3dImmediateContext->OMSetRenderTargets( 1, &g_hdrBackbufferRTV, pDSV );
	pd3dImmediateContext->ClearRenderTargetView( g_hdrBackbufferRTV, Colors::Black );
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	pd3dImmediateContext->RSSetState( g_rasterizerState );
	pd3dImmediateContext->OMSetDepthStencilState( g_depthStencilState, 0 );
	pd3dImmediateContext->PSSetSamplers( LINEAR_WRAP_SAMPLER_STATE, 1, &g_linearWrapSamplerState );

	// update global params
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubres;
		pd3dImmediateContext->Map( g_globalParamsBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );

		CBaseCamera* currentCamera = g_sceneType == SCENE_ONE_SPHERE ? ( CBaseCamera* )&g_modelViewerCamera : ( CBaseCamera* )&g_firstPersonCamera;

		GlobalParams* globalParams = ( GlobalParams* )mappedSubres.pData;
		globalParams->ViewProjMatrix = XMMatrixMultiply( currentCamera->GetViewMatrix(), currentCamera->GetProjMatrix() );
		globalParams->ViewPos = currentCamera->GetEyePt();
		float lightDirVert = ToRad( ( float )g_lightDirVert );
		float lightDirHor = ToRad( ( float )g_lightDirHor );
		globalParams->LightDir = XMVectorSet( sin( lightDirVert ) * sin( lightDirHor ), cos( lightDirVert ), sin( lightDirVert ) * cos( lightDirHor ), 0.0f );
		globalParams->FrameIdx = g_frameIdx;

		pd3dImmediateContext->Unmap( g_globalParamsBuf, 0 );

		// apply
		pd3dImmediateContext->VSSetConstantBuffers( GLOBAL_PARAMS_CB, 1, &g_globalParamsBuf );
		pd3dImmediateContext->PSSetConstantBuffers( GLOBAL_PARAMS_CB, 1, &g_globalParamsBuf );
	}


	//
	ID3D11ShaderResourceView* environmentMap = nullptr;
	pd3dImmediateContext->PSSetShaderResources( ENVIRONMENT_MAP, 1, &environmentMap );
	g_skyRenderer.BakeCubemap( pd3dImmediateContext );

	if( g_drawSky )
		g_skyRenderer.Render( pd3dImmediateContext );

	environmentMap = g_skyRenderer.GetCubeMapSRV();
	pd3dImmediateContext->PSSetShaderResources( ENVIRONMENT_MAP, 1, &environmentMap );

	if( g_sceneType == SCENE_ONE_SPHERE )
	{
		g_sphereRenderer.Render( XMMatrixIdentity(), g_metalness, g_roughness, g_directLight, g_indirectLight, pd3dImmediateContext );
	} 
	else if( g_sceneType == SCENE_MULTIPLE_SPHERES )
	{
		for( int x = -5; x <= 5; x++ )
		{
			for( int z = -5; z <= 5; z++ )
			{
				float metalness = ( x + 5 ) / 10.0f;
				float roughness = ( z + 5 ) / 10.0f;
				XMMATRIX tranlation = XMMatrixTranslation( x * 2.5f, 0.0f, z * 2.5f );
				g_sphereRenderer.Render( tranlation, metalness, roughness, g_directLight, g_indirectLight, pd3dImmediateContext );
			}
		}
	}

	//
	pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV, pDSV );
	pd3dImmediateContext->ClearRenderTargetView( pRTV, Colors::MidnightBlue );
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	//g_postProcess.Render( pd3dImmediateContext, g_hdrBackbufferSRV, g_exposure );

	{
		D3D11_MAPPED_SUBRESOURCE mappedSubres;
		pd3dImmediateContext->Map( g_lineParamsBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );

		LineParams* lineParams = ( LineParams* )mappedSubres.pData;
		lineParams->Metalness = g_metalness;
		lineParams->Roughness = g_roughness;
		lineParams->NumSamples = 1024;
		pd3dImmediateContext->Unmap( g_lineParamsBuf, 0 );

		// apply
		pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &g_lineParamsBuf );
		pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &g_lineParamsBuf );
		pd3dImmediateContext->IASetInputLayout( nullptr );
		pd3dImmediateContext->VSSetShader( g_lineVs, nullptr, 0 );
		pd3dImmediateContext->PSSetShader( g_linePs, nullptr, 0 );
		pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
		pd3dImmediateContext->Draw( lineParams->NumSamples * 2 + 4, 0 );

		pd3dImmediateContext->VSSetShader( g_planeVs, nullptr, 0 );
		pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		pd3dImmediateContext->DrawInstanced( 6, 2, 0, 0 );
	}

	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
	g_HUD.OnRender(fElapsedTime);
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
	SAFE_RELEASE( g_hdrBackbufferTexture );
	SAFE_RELEASE( g_hdrBackbufferRTV );
	SAFE_RELEASE( g_hdrBackbufferSRV );
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
	SAFE_RELEASE( g_linearWrapSamplerState );
	SAFE_RELEASE( g_hdrBackbufferTexture );
	SAFE_RELEASE( g_hdrBackbufferRTV );
	SAFE_RELEASE( g_hdrBackbufferSRV );


	g_sphereRenderer.OnD3D11DestroyDevice();
	g_skyRenderer.OnD3D11DestroyDevice();
	g_postProcess.OnD3D11DestroyDevice();
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

	// Pass all remaining windows messages to camera so it can respond to user input
	if( g_sceneType == SCENE_ONE_SPHERE )
		g_modelViewerCamera.HandleMessages( hWnd, uMsg, wParam, lParam );
	else 
		g_firstPersonCamera.HandleMessages( hWnd, uMsg, wParam, lParam );

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
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1920, 1080 );
    DXUTMainLoop(); // Enter into the DXUT ren  der loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}


void InitApp()
{
	g_D3DSettingsDlg.Init(&g_DialogResourceManager);
	g_HUD.Init( &g_DialogResourceManager );
	g_HUD.SetCallback( OnGUIEvent );

	int iY = 10;
	g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY, HUD_WIDTH, 23, VK_F2 );
	g_HUD.AddButton( IDC_RELOAD_SHADERS, L"Reload shaders (F3)", 0, iY += 25, HUD_WIDTH, 23, VK_F3 );

	iY += 50;
	WCHAR str[MAX_PATH];
	swprintf_s( str, MAX_PATH, L"Light vert: %d", g_lightDirVert );
	g_HUD.AddStatic( IDC_LIGHTVERT_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_LIGHTVERT, 0, iY += 24, HUD_WIDTH, 22, 0, 180, g_lightDirVert );

	swprintf_s( str, MAX_PATH, L"Light hor: %d", g_lightDirHor );
	g_HUD.AddStatic(IDC_LIGHTHOR_STATIC, str, 25, iY += 24, HUD_WIDTH, 22);
	g_HUD.AddSlider(IDC_LIGHTHOR, 0, iY += 24, HUD_WIDTH, 22, 0, 180, g_lightDirHor );

	swprintf_s( str, MAX_PATH, L"Exposure: %1.2f", g_exposure );
	g_HUD.AddStatic( IDC_EXPOSURE_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_EXPOSURE, 0, iY += 24, HUD_WIDTH, 22, 0, 100, ( int )( g_exposure * 20.0f ) );

	swprintf_s( str, MAX_PATH, L"Draw sky" );
	g_HUD.AddCheckBox( IDC_DRAW_SKY, str, 0, iY += 24, HUD_WIDTH, 22, g_drawSky );

	CDXUTComboBox* skyTextureComboxBox;
	g_HUD.AddComboBox( IDC_SELECT_SKY_TEXTURE, 0, iY += 24, HUD_WIDTH, 22, 0, false, &skyTextureComboxBox );
	for( size_t i = 0; i < sizeof( g_skyTextureFiles ) / sizeof( wchar_t* ); i++ )
		skyTextureComboxBox->AddItem( g_skyTextureFiles[ i ], ( void* )g_skyTextureFiles[ i ] );

	CDXUTComboBox* sceneComboBox;
	g_HUD.AddComboBox( IDC_SCENE_TYPE, 0, iY += 24, HUD_WIDTH, 22, 0, false, &sceneComboBox );
	sceneComboBox->AddItem( L"One sphere", ULongToPtr( SCENE_ONE_SPHERE ) );
	sceneComboBox->AddItem( L"Multiple spheres", ULongToPtr( SCENE_MULTIPLE_SPHERES ) );
	//sceneComboBox->AddItem( L"Sponza", ULongToPtr( SCENE_SPONZA ) );

	swprintf_s( str, MAX_PATH, L"Metalness: %1.2f", g_metalness );
	g_HUD.AddStatic( IDC_METALNESS_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_METALNESS, 0, iY += 24, HUD_WIDTH, 22, 0, 100, ( int )( g_metalness * 100.0f ) );

	swprintf_s( str, MAX_PATH, L"Roughness: %1.2f", g_roughness );
	g_HUD.AddStatic( IDC_ROUGHNESS_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_ROUGHNESS, 0, iY += 24, HUD_WIDTH, 22, 0, 100, ( int )( g_roughness * 100.0f ) );

	swprintf_s( str, MAX_PATH, L"Enable direct light" );
	g_HUD.AddCheckBox( IDC_DIRECT_LIGHT, str, 0, iY += 24, HUD_WIDTH, 22, g_directLight );

	swprintf_s( str, MAX_PATH, L"Enable indirect light" );
	g_HUD.AddCheckBox( IDC_INDIRECT_LIGHT, str, 0, iY += 24, HUD_WIDTH, 22, g_indirectLight );
}


void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	WCHAR str[MAX_PATH];

	switch (nControlID)
	{
		case IDC_CHANGEDEVICE:
		{
			g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() );
			break;
		}
		case IDC_RELOAD_SHADERS:
		{
			g_skyRenderer.ReloadShaders( DXUTGetD3D11Device() );
			g_sphereRenderer.ReloadShaders( DXUTGetD3D11Device() );
			g_postProcess.ReloadShaders( DXUTGetD3D11Device() );
			break;
		}
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
		case IDC_DIRECT_LIGHT:
		{
			g_directLight = g_HUD.GetCheckBox( IDC_DIRECT_LIGHT )->GetChecked();
			break;
		}
		case IDC_INDIRECT_LIGHT:
		{
			g_indirectLight = g_HUD.GetCheckBox( IDC_INDIRECT_LIGHT )->GetChecked();
			break;
		}
		case IDC_EXPOSURE:
		{
			g_exposure = g_HUD.GetSlider( IDC_EXPOSURE )->GetValue() / 20.0f;
			swprintf_s( str, MAX_PATH, L"Exposure: %1.2f", g_exposure );
			g_HUD.GetStatic( IDC_EXPOSURE_STATIC )->SetText( str );
			break;
		}
		case IDC_DRAW_SKY:
		{
			g_drawSky = g_HUD.GetCheckBox( IDC_DRAW_SKY )->GetChecked();
			break;
		}
		case IDC_SELECT_SKY_TEXTURE:
		{
			const wchar_t* filename = ( const wchar_t* )g_HUD.GetComboBox( IDC_SELECT_SKY_TEXTURE )->GetSelectedData();
			g_skyRenderer.LoadSkyTexture( DXUTGetD3D11Device(), filename );
		}
		case IDC_SCENE_TYPE:
		{
			g_sceneType = ( SCENE_TYPE )PtrToUlong( g_HUD.GetComboBox( IDC_SCENE_TYPE )->GetSelectedData() );
			break;
		}
	}
}


