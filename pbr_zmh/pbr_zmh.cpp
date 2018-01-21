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
	UINT TotalSamples;
	UINT SamplesInStep;
	UINT SamplesProcessed;
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
	IDC_USE_MATERIAL,
	IDC_MATERIAL,
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
ID3D11DepthStencilState*			g_depthPassDepthStencilState = nullptr;
ID3D11DepthStencilState*			g_lightPassDepthStencilState = nullptr;
ID3D11RasterizerState*				g_rasterizerState = nullptr;
ID3D11BlendState*					g_singleRtBlendState = nullptr;
ID3D11BlendState*					g_doubleRtBlendState = nullptr;
ID3D11SamplerState*					g_linearWrapSamplerState = nullptr;
SphereRenderer						g_sphereRenderer;
SkyRenderer							g_skyRenderer;
PostProcess							g_postProcess;

// lighting render targets 
RenderTarget						g_directLightRenderTarget;
RenderTarget						g_indirectLightRenderTarget;
Material							g_material;

// UI stuff
int g_lightDirVert = 45;
int g_lightDirHor = 130;
float g_metalness = 1.0f;
float g_roughness = 0.5f;
bool g_useMaterial = false;
bool g_enableDirectLight = true;
bool g_enableIndirectLight = true;
float g_exposure = 1.0f;
bool g_drawSky = true;
SCENE_TYPE g_sceneType = SCENE_ONE_SPHERE;

// Image base lighting importance sampling stuff
UINT g_frameIdx = 0;
UINT g_samplesProcessed = 0;
bool g_resetSampling = false;
XMMATRIX g_lastFrameViewProj;
const UINT TotalSamples = 128;
const UINT SamplesInStep = 16;

const wchar_t* g_skyTextureFiles[] = {
	L"HDRs\\galileo_cross.dds",
	L"HDRs\\grace_cross.dds",
	L"HDRs\\rnl_cross.dds",
	L"HDRs\\stpeters_cross.dds",
	L"HDRs\\uffizi_cross.dds",
};

const wchar_t* g_materials[] = {
	L"materials\\default",
	L"materials\\Brick_baked",
	L"materials\\Brick_Beige",
	L"materials\\Brick_Cinder",
	L"materials\\Brick_Concrete",
	L"materials\\Brick_Yellow",
	L"materials\\Concrete_Shimizu",
	L"materials\\Concrete_Tadao",
	L"materials\\copper-oxidized",
	L"materials\\copper-rock",
	L"materials\\copper-scuffed",
	L"materials\\greasy-pan",
	L"materials\\OldPlaster_00",
	L"materials\\OldPlaster_01",
	L"materials\\rustediron",
	L"materials\\streakedmetal",
	L"materials\\T_Paint_Black",
	L"materials\\T_Tile_White",
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
	pd3dDevice->CreateDepthStencilState( &depthDesc, &g_depthPassDepthStencilState );

	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
	pd3dDevice->CreateDepthStencilState( &depthDesc, &g_lightPassDepthStencilState );

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

	D3D11_BLEND_DESC blendDesc;
	memset( &blendDesc, 0, sizeof( D3D11_BLEND_DESC ) );
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = true;
	blendDesc.RenderTarget[ 0 ].RenderTargetWriteMask = 0x0f;
	V_RETURN( pd3dDevice->CreateBlendState( &blendDesc, &g_singleRtBlendState ) );

	blendDesc.RenderTarget[ 1 ].BlendEnable = true;
	blendDesc.RenderTarget[ 1 ].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[ 1 ].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[ 1 ].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[ 1 ].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[ 1 ].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[ 1 ].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[ 1 ].RenderTargetWriteMask = 0x0f;
	V_RETURN( pd3dDevice->CreateBlendState( &blendDesc, &g_doubleRtBlendState ) );

	g_material.Load( pd3dDevice, L"materials\\default" );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	HRESULT hr = S_OK;

	g_directLightRenderTarget.Init(pd3dDevice, DXGI_FORMAT_R16G16B16A16_FLOAT, pBackBufferSurfaceDesc->Width,
		pBackBufferSurfaceDesc->Height, "DirectLightRT");
	g_indirectLightRenderTarget.Init(pd3dDevice, DXGI_FORMAT_R32G32B32A32_FLOAT, pBackBufferSurfaceDesc->Width,
		pBackBufferSurfaceDesc->Height, "IndirectLightRT");

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

	g_resetSampling = true;

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


void BakeCubeMap( ID3D11DeviceContext* pd3dImmediateContext )
{
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Bake cubemap" );
	ID3D11ShaderResourceView* environmentMap = nullptr;
	pd3dImmediateContext->PSSetShaderResources( ENVIRONMENT_MAP, 1, &environmentMap );
	g_skyRenderer.BakeCubemap( pd3dImmediateContext );
	DXUT_EndPerfEvent();
}


void RenderScene( ID3D11DeviceContext* pd3dImmediateContext )
{
	SphereRenderer::InstanceParams oneSphereInstance;
	SphereRenderer::InstanceParams multSphereInstances[ 11 * 11 ];
	oneSphereInstance.WorldMatrix = XMMatrixIdentity();
	oneSphereInstance.Metalness = g_metalness;
	oneSphereInstance.Roughness = g_roughness;
	oneSphereInstance.EnableDirectLight = g_enableDirectLight;
	oneSphereInstance.EnableIndirectLight = g_enableIndirectLight;
	oneSphereInstance.UseMaterial = g_useMaterial;
	int instanceCounter = 0;
	for( int x = -5; x <= 5; x++ )
	{
		for( int z = -5; z <= 5; z++ )
		{
			multSphereInstances[ instanceCounter ] = oneSphereInstance;
			multSphereInstances[ instanceCounter ].Metalness = ( x + 5 ) / 10.0f;
			multSphereInstances[ instanceCounter ].Roughness = ( z + 5 ) / 10.0f;
			multSphereInstances[ instanceCounter ].WorldMatrix = XMMatrixTranslation( x * 2.5f, 0.0f, z * 2.5f );
			multSphereInstances[ instanceCounter ].UseMaterial = false;
			instanceCounter++;
		}
	}
	SphereRenderer::InstanceParams* sphereInstances = g_sceneType == SCENE_ONE_SPHERE ? &oneSphereInstance : &multSphereInstances[ 0 ];
	UINT numSphereInstances = g_sceneType == SCENE_ONE_SPHERE ? 1 : 11 * 11;

	// depth pass
	{
		DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Depth pass" );
		ID3D11RenderTargetView* rtv[2] = { nullptr, nullptr };
		pd3dImmediateContext->OMSetRenderTargets( 2, rtv, DXUTGetD3D11DepthStencilView() );
		pd3dImmediateContext->ClearDepthStencilView( DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0, 0 );
		pd3dImmediateContext->OMSetDepthStencilState( g_depthPassDepthStencilState, 0 );

		if( g_drawSky )
			g_skyRenderer.RenderDepthPass( pd3dImmediateContext );
		if( g_sceneType == SCENE_ONE_SPHERE || g_sceneType == SCENE_MULTIPLE_SPHERES )
			g_sphereRenderer.RenderDepthPass( sphereInstances, numSphereInstances, pd3dImmediateContext );

		DXUT_EndPerfEvent();
	}

	// light pass
	{
		DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Light pass" );
		ID3D11RenderTargetView* rtv[2];
		rtv[ 0 ] = g_directLightRenderTarget.rtv;
		rtv[ 1 ] = g_indirectLightRenderTarget.rtv;
		pd3dImmediateContext->OMSetRenderTargets( 2, rtv, DXUTGetD3D11DepthStencilView() );
		pd3dImmediateContext->ClearRenderTargetView( g_directLightRenderTarget.rtv, Colors::Black );
		pd3dImmediateContext->OMSetDepthStencilState( g_lightPassDepthStencilState, 0 );

		ID3D11ShaderResourceView* environmentMap = g_skyRenderer.GetCubeMapSRV();
		pd3dImmediateContext->PSSetShaderResources( ENVIRONMENT_MAP, 1, &environmentMap );
		pd3dImmediateContext->OMSetBlendState( g_doubleRtBlendState, nullptr, ~0u );

		if( g_drawSky )
			g_skyRenderer.RenderLightPass( pd3dImmediateContext );
		if( g_sceneType == SCENE_ONE_SPHERE || g_sceneType == SCENE_MULTIPLE_SPHERES )
		{
			Material* material = g_useMaterial ? &g_material : nullptr;
			g_sphereRenderer.RenderLightPass( sphereInstances, material, numSphereInstances, pd3dImmediateContext );
		}

		DXUT_EndPerfEvent();
	}

	if( g_samplesProcessed < TotalSamples )
			g_samplesProcessed += SamplesInStep;
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
	CBaseCamera* currentCamera = g_sceneType == SCENE_ONE_SPHERE ? (CBaseCamera*)&g_modelViewerCamera : (CBaseCamera*)&g_firstPersonCamera;
	XMMATRIX viewProj = XMMatrixMultiply(currentCamera->GetViewMatrix(), currentCamera->GetProjMatrix());

	if( g_samplesProcessed == 0 || g_resetSampling || memcmp( &viewProj, &g_lastFrameViewProj, sizeof( XMMATRIX ) ) != 0 ) {
		pd3dImmediateContext->ClearRenderTargetView( g_indirectLightRenderTarget.rtv, Colors::Black );
		g_lastFrameViewProj = viewProj;
		g_resetSampling = false;
		g_samplesProcessed = 0;
	}

	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if (g_D3DSettingsDlg.IsActive())
	{
		g_D3DSettingsDlg.OnRender(fElapsedTime);
		return;
	}

	pd3dImmediateContext->RSSetState( g_rasterizerState );
	pd3dImmediateContext->OMSetDepthStencilState( g_lightPassDepthStencilState, 0 );
	pd3dImmediateContext->OMSetBlendState( g_singleRtBlendState, nullptr, ~0u );
	pd3dImmediateContext->PSSetSamplers( LINEAR_WRAP_SAMPLER_STATE, 1, &g_linearWrapSamplerState );

	// update global params
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubres;
		pd3dImmediateContext->Map( g_globalParamsBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );

		GlobalParams* globalParams = ( GlobalParams* )mappedSubres.pData;
		globalParams->ViewProjMatrix = viewProj;
		globalParams->ViewPos = currentCamera->GetEyePt();
		float lightDirVert = ToRad( ( float )g_lightDirVert );
		float lightDirHor = ToRad( ( float )g_lightDirHor );
		globalParams->LightDir = XMVectorSet( sin( lightDirVert ) * sin( lightDirHor ), cos( lightDirVert ), sin( lightDirVert ) * cos( lightDirHor ), 0.0f );
		globalParams->FrameIdx = g_frameIdx;
		globalParams->TotalSamples = TotalSamples;
		globalParams->SamplesInStep = SamplesInStep;
		globalParams->SamplesProcessed = g_samplesProcessed;

		pd3dImmediateContext->Unmap( g_globalParamsBuf, 0 );

		// apply
		pd3dImmediateContext->VSSetConstantBuffers( GLOBAL_PARAMS_CB, 1, &g_globalParamsBuf );
		pd3dImmediateContext->PSSetConstantBuffers( GLOBAL_PARAMS_CB, 1, &g_globalParamsBuf );
	}

	BakeCubeMap( pd3dImmediateContext );
	RenderScene( pd3dImmediateContext );	

	//
	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"Tonemap");
	ID3D11RenderTargetView* rtv[2];
	rtv[ 0 ] = pRTV;
	rtv[ 1 ] = nullptr;
	pd3dImmediateContext->OMSetRenderTargets( 2, rtv, pDSV );
	pd3dImmediateContext->ClearRenderTargetView( pRTV, Colors::MidnightBlue );
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );
	g_postProcess.Render( pd3dImmediateContext, g_directLightRenderTarget.srv, g_indirectLightRenderTarget.srv, g_exposure );
	DXUT_EndPerfEvent();

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
	g_directLightRenderTarget.Release();
	g_indirectLightRenderTarget.Release();
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
	SAFE_RELEASE( g_depthPassDepthStencilState );
	SAFE_RELEASE( g_lightPassDepthStencilState );
	SAFE_RELEASE( g_singleRtBlendState );
	SAFE_RELEASE( g_doubleRtBlendState );
	SAFE_RELEASE( g_linearWrapSamplerState );
	g_directLightRenderTarget.Release();
	g_indirectLightRenderTarget.Release();
	g_material.Release();

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

	swprintf_s( str, MAX_PATH, L"Use material" );
	g_HUD.AddCheckBox( IDC_USE_MATERIAL, str, 0, iY += 24, HUD_WIDTH, 22, g_useMaterial );

	CDXUTComboBox* materialComboxBox;
	g_HUD.AddComboBox( IDC_MATERIAL, 0, iY += 24, HUD_WIDTH, 22, 0, false, &materialComboxBox );
	for (size_t i = 0; i < sizeof( g_materials ) / sizeof( wchar_t* ); i++ )
		materialComboxBox->AddItem( g_materials[ i ], (void*)g_materials[ i ] );

	swprintf_s( str, MAX_PATH, L"Enable direct light" );
	g_HUD.AddCheckBox( IDC_DIRECT_LIGHT, str, 0, iY += 24, HUD_WIDTH, 22, g_enableDirectLight );

	swprintf_s( str, MAX_PATH, L"Enable indirect light" );
	g_HUD.AddCheckBox( IDC_INDIRECT_LIGHT, str, 0, iY += 24, HUD_WIDTH, 22, g_enableIndirectLight );
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
			g_resetSampling = true;
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
			g_resetSampling = true;
			break;
		}
		case IDC_ROUGHNESS:
		{
			g_roughness = g_HUD.GetSlider( IDC_ROUGHNESS )->GetValue() / 100.0f;
			swprintf_s( str, MAX_PATH, L"Roughness: %1.2f", g_roughness );
			g_HUD.GetStatic( IDC_ROUGHNESS_STATIC )->SetText( str );
			g_resetSampling = true;
			break;
		}
		case IDC_USE_MATERIAL:
		{
			g_useMaterial = g_HUD.GetCheckBox( IDC_USE_MATERIAL )->GetChecked();
			g_resetSampling = true;
			break;
		}
		case IDC_MATERIAL:
		{
			const wchar_t* filename = ( const wchar_t* )g_HUD.GetComboBox( IDC_MATERIAL )->GetSelectedData();
			g_material.Load( DXUTGetD3D11Device(), filename );
			g_resetSampling = true;
		}
		case IDC_DIRECT_LIGHT:
		{
			g_enableDirectLight = g_HUD.GetCheckBox( IDC_DIRECT_LIGHT )->GetChecked();
			break;
		}
		case IDC_INDIRECT_LIGHT:
		{
			g_enableIndirectLight = g_HUD.GetCheckBox( IDC_INDIRECT_LIGHT )->GetChecked();
			g_resetSampling = true;
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
			g_resetSampling = true;
		}
		case IDC_SCENE_TYPE:
		{
			g_sceneType = ( SCENE_TYPE )PtrToUlong( g_HUD.GetComboBox( IDC_SCENE_TYPE )->GetSelectedData() );
			g_resetSampling = true;
			break;
		}
	}
}
