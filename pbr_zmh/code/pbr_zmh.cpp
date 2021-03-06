#include "Precompiled.h"
#include "resource.h"
#include "imgui/imgui.h"

#define GLOBAL_PARAMS_CB 0
#define PREFILTERED_DIFF_ENV_MAP 123
#define BRDF_LUT 124
#define PREFILTERED_SPEC_ENV_MAP 125
#define SHADOW_MAP 126
#define ENVIRONMENT_MAP 127
#define LINEAR_CLAMP_SAMPLER_STATE 13
#define CMP_LINEAR_SAMPLER_STATE 14
#define LINEAR_WRAP_SAMPLER_STATE 15

#pragma warning( disable : 4100 )

using namespace DirectX;


//
__declspec( align( 16 ) )
struct GlobalParams
{
	XMMATRIX ViewProjMatrix;
	XMVECTOR ViewPos;
	XMVECTOR LightDir;
	XMVECTOR LightIlluminance;
	XMMATRIX ShadowMatrix;
	float IndirectLightIntensity;
	UINT ApproxLevel;
	UINT FrameIdx;
	UINT TotalSamples;
	UINT SamplesInStep;
	UINT SamplesProcessed;
	UINT EnableDirectLight;
	UINT EnableIndirectLight;
	UINT EnableShadow;
	UINT EnableDiffuseLight;
	UINT EnableSpecularLight;
	UINT ScreenWidth;
	UINT ScreenHeight;
};


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CModelViewerCamera                  g_modelViewerCamera;     // A model viewing camera
CFirstPersonCamera					g_firstPersonCamera;

ID3D11Buffer*						g_globalParamsBuf = nullptr;
GlobalParams						g_cachedGlobalParams;
ID3D11DepthStencilState*			g_depthPassDepthStencilState = nullptr;
ID3D11DepthStencilState*			g_lightPassDepthStencilState = nullptr;
ID3D11RasterizerState*				g_rasterizerState = nullptr;
ID3D11BlendState*					g_singleRtBlendState = nullptr;
ID3D11BlendState*					g_doubleRtBlendState = nullptr;
ID3D11SamplerState*					g_cmpLinearSamplerState = nullptr;
ID3D11SamplerState*					g_linearWrapSamplerState = nullptr;
ID3D11SamplerState*					g_linearClampSamplerState = nullptr;
SphereRenderer						g_sphereRenderer;
SkyRenderer							g_skyRenderer;
SponzaRenderer						g_sponzaRenderer;
PostProcess							g_postProcess;
EnvMapFilter						g_envMapFilter;

// lighting render targets 
RenderTarget						g_directLightRenderTarget;
RenderTarget						g_indirectLightRenderTarget;
DepthRenderTarget					g_shadowMap;
Material							g_material;
MERLMaterial						g_merlMaterial;

// Image base lighting importance sampling stuff
bool g_resetSampling = false;
XMMATRIX g_lastFrameViewProj;
const UINT SHADOW_MAP_RESOLUTION = 2048;


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
	V_RETURN( UIOnDeviceCreate( DXUTGetHWND(), pd3dDevice, pd3dImmediateContext ) );

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
	V_RETURN( g_skyRenderer.LoadSkyTexture( pd3dDevice, GetGlobalControls().skyTexture ) );
	V_RETURN( g_postProcess.OnD3D11CreateDevice( pd3dDevice ) );
	V_RETURN( g_sponzaRenderer.OnD3D11CreateDevice( pd3dDevice ) );

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
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN( pd3dDevice->CreateSamplerState( &samplerDesc, &g_linearWrapSamplerState ) );

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	V_RETURN( pd3dDevice->CreateSamplerState( &samplerDesc, &g_linearClampSamplerState ) );

	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	samplerDesc.MaxLOD = 0.0f;
	V_RETURN( pd3dDevice->CreateSamplerState( &samplerDesc, &g_cmpLinearSamplerState ) );

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

	g_shadowMap.Init( pd3dDevice, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, "ShadowMap" );

	g_envMapFilter.OnD3D11CreateDevice( pd3dDevice );

	g_cachedGlobalParams.SamplesProcessed = 0;

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

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_modelViewerCamera.SetProjParams( 53.4f * ( XM_PI / 180.0f ), fAspectRatio, 0.1f, 3000.0f );
	g_modelViewerCamera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
	g_modelViewerCamera.SetButtonMasks( 0, MOUSE_WHEEL, MOUSE_RIGHT_BUTTON | MOUSE_LEFT_BUTTON );
	g_firstPersonCamera.SetProjParams( 53.4f * ( XM_PI / 180.0f ), fAspectRatio, 0.1f, 3000.0f );

	g_cachedGlobalParams.ScreenWidth = pBackBufferSurfaceDesc->Width;
	g_cachedGlobalParams.ScreenHeight = pBackBufferSurfaceDesc->Height;

	g_resetSampling = true;

	V_RETURN( UIOnResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	// Update the camera's position based on user input 
	if( GetGlobalControls().sceneType == SCENE_ONE_SPHERE )
		g_modelViewerCamera.FrameMove( fElapsedTime );
	else
		g_firstPersonCamera.FrameMove( fElapsedTime );
}


void FlushGlobalParams( ID3D11DeviceContext* pd3dImmediateContext )
{
	D3D11_MAPPED_SUBRESOURCE mappedSubres;
	pd3dImmediateContext->Map( g_globalParamsBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );
	memcpy( mappedSubres.pData, &g_cachedGlobalParams, sizeof( GlobalParams ) );
	pd3dImmediateContext->Unmap( g_globalParamsBuf, 0 );
}


void BakeEnvMap( ID3D11DeviceContext* pd3dImmediateContext )
{
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Bake envmap" );
	ID3D11ShaderResourceView* environmentMap = nullptr;
	pd3dImmediateContext->PSSetShaderResources( ENVIRONMENT_MAP, 1, &environmentMap );
	g_skyRenderer.BakeCubemap( pd3dImmediateContext );
	DXUT_EndPerfEvent();
}


void PrefilterEnvMap( ID3D11DeviceContext* pd3dImmediateContext )
{
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Prefilter env map" );
	ID3D11ShaderResourceView* nullSrv = nullptr;
	pd3dImmediateContext->PSSetShaderResources( PREFILTERED_SPEC_ENV_MAP, 1, &nullSrv );
	pd3dImmediateContext->PSSetShaderResources( BRDF_LUT, 1, &nullSrv );
	pd3dImmediateContext->PSSetShaderResources( PREFILTERED_DIFF_ENV_MAP, 1, &nullSrv );
	g_envMapFilter.FilterEnvMap( pd3dImmediateContext, ENVIRONMENT_MAP, g_skyRenderer.GetCubeMapSRV() );
	DXUT_EndPerfEvent();
}


void ComputeShadowMatrix()
{
	float lightDirVert = ToRad( ( float )GetGlobalControls().lightDirVert );
	float lightDirHor = ToRad( ( float )GetGlobalControls().lightDirHor );
	XMVECTOR lightDir = XMVectorSet( sin( lightDirVert ) * sin( lightDirHor ), cos( lightDirVert ), sin( lightDirVert ) * cos( lightDirHor ), 0.0f );

	const float width = 40.0f;
	const float height = 40.0f;
	const float depthRange = 100.0f;
	XMVECTOR shadowCameraPos = XMVectorScale( lightDir, 0.5f * depthRange );

	XMMATRIX viewMatrix, projMatrix;
	viewMatrix = XMMatrixLookAtLH( shadowCameraPos, XMVectorZero(), XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ) );
	projMatrix = XMMatrixOrthographicLH( width, height, 1.0f, depthRange );

	XMMATRIX lsOffset;
	lsOffset.r[ 0 ] = XMVectorSet( 0.5f,  0.0f, 0.0f, 0.0f );
	lsOffset.r[ 1 ] = XMVectorSet( 0.0f, -0.5f, 0.0f, 0.0f );
	lsOffset.r[ 2 ] = XMVectorSet( 0.0f,  0.0f, 1.0f, 0.0f );
	lsOffset.r[ 3 ] = XMVectorSet( 0.5f,  0.5f, 0.0f, 1.0f );

	g_cachedGlobalParams.ViewProjMatrix = XMMatrixMultiply( viewMatrix, projMatrix );
	g_cachedGlobalParams.ShadowMatrix = XMMatrixMultiply( g_cachedGlobalParams.ViewProjMatrix, lsOffset );
}


void RenderScene( ID3D11DeviceContext* pd3dImmediateContext, float fTime )
{
	SphereRenderer::InstanceParams oneSphereInstance;
	SphereRenderer::InstanceParams multSphereInstances[ 11 * 11 ];
	oneSphereInstance.WorldMatrix = XMMatrixIdentity();
	oneSphereInstance.Metalness = GetOneSphereSceneControls().metalness;
	oneSphereInstance.Roughness = GetOneSphereSceneControls().roughness;
	oneSphereInstance.Reflectance = GetOneSphereSceneControls().reflectance;
	oneSphereInstance.Albedo = GetOneSphereSceneControls().albedo;
	oneSphereInstance.MaterialType = GetOneSphereSceneControls().materialType;
	int instanceCounter = 0;
	for( int x = -5; x <= 5; x++ )
	{
		for( int z = -5; z <= 5; z++ )
		{
			multSphereInstances[ instanceCounter ].WorldMatrix = XMMatrixTranslation( x * 2.5f, 0.0f, z * 2.5f );
			multSphereInstances[ instanceCounter ].Metalness = ( x + 5 ) / 10.0f;
			multSphereInstances[ instanceCounter ].Roughness = ( z + 5 ) / 10.0f;
			multSphereInstances[ instanceCounter ].Reflectance = 1.0f;
			multSphereInstances[ instanceCounter ].Albedo = GetMultipleSphereSceneControls().albedo;
			multSphereInstances[ instanceCounter ].MaterialType = MATERIAL_SIMPLE;
			instanceCounter++;
		}
	}
	SphereRenderer::InstanceParams* sphereInstances = GetGlobalControls().sceneType == SCENE_ONE_SPHERE ? &oneSphereInstance : &multSphereInstances[ 0 ];
	UINT numSphereInstances = GetGlobalControls().sceneType == SCENE_ONE_SPHERE ? 1 : 11 * 11;

	XMMATRIX metalnessPlane = XMMatrixTranslation( -11.0f, 0.0f, -15.0f );
	XMMATRIX rotation = XMMatrixRotationY( XM_PIDIV2 + XM_PI );
	XMMATRIX roughnessPlane = XMMatrixMultiply( rotation, XMMatrixTranslation( -15.0f, 0.0f, -11.0f ) );

	// shadow pass
	{
		DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Shadow pass" );
		ID3D11ShaderResourceView* nullSrv = nullptr;
		pd3dImmediateContext->PSSetShaderResources( SHADOW_MAP, 1, &nullSrv );
		ID3D11RenderTargetView* rtv[ 2 ] = { nullptr, nullptr };
		pd3dImmediateContext->OMSetRenderTargets( 2, rtv, g_shadowMap.dsv );
		pd3dImmediateContext->ClearDepthStencilView( g_shadowMap.dsv, D3D11_CLEAR_DEPTH, 1.0, 0 );
		pd3dImmediateContext->OMSetDepthStencilState( g_depthPassDepthStencilState, 0 );

		D3D11_VIEWPORT savedViewport;
		UINT numViewports = 1;
		pd3dImmediateContext->RSGetViewports( &numViewports, &savedViewport );
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = SHADOW_MAP_RESOLUTION;
		viewport.Height = SHADOW_MAP_RESOLUTION;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		pd3dImmediateContext->RSSetViewports( 1, &viewport );

		XMMATRIX savedViewProj = g_cachedGlobalParams.ViewProjMatrix;
		ComputeShadowMatrix();
		FlushGlobalParams( pd3dImmediateContext );

		if( GetGlobalControls().sceneType == SCENE_ONE_SPHERE || GetGlobalControls().sceneType == SCENE_MULTIPLE_SPHERES )
			g_sphereRenderer.RenderDepthPass( sphereInstances, numSphereInstances, pd3dImmediateContext );
		if( GetGlobalControls().sceneType == SCENE_SPONZA )
			g_sponzaRenderer.RenderDepthPass( pd3dImmediateContext );

		g_cachedGlobalParams.ViewProjMatrix = savedViewProj;
		FlushGlobalParams( pd3dImmediateContext );

		pd3dImmediateContext->RSSetViewports( 1, &savedViewport );

		DXUT_EndPerfEvent();
	}

	// depth pass
	{
		DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Depth pass" );
		ID3D11RenderTargetView* rtv[2] = { nullptr, nullptr };
		pd3dImmediateContext->OMSetRenderTargets( 2, rtv, DXUTGetD3D11DepthStencilView() );
		pd3dImmediateContext->ClearDepthStencilView( DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0, 0 );
		pd3dImmediateContext->OMSetDepthStencilState( g_depthPassDepthStencilState, 0 );

		if( GetGlobalControls().drawSky )
			g_skyRenderer.RenderDepthPass( pd3dImmediateContext );
		if( GetGlobalControls().sceneType == SCENE_ONE_SPHERE || GetGlobalControls().sceneType == SCENE_MULTIPLE_SPHERES )
			g_sphereRenderer.RenderDepthPass( sphereInstances, numSphereInstances, pd3dImmediateContext );
		if( GetGlobalControls().sceneType == SCENE_SPONZA )
			g_sponzaRenderer.RenderDepthPass( pd3dImmediateContext );

		if( GetGlobalControls().sceneType == SCENE_MULTIPLE_SPHERES )
			g_sphereRenderer.RenderPlaneDepthPass( metalnessPlane, roughnessPlane, pd3dImmediateContext );

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
		ID3D11ShaderResourceView* prefilteredSpecEnvMap = g_envMapFilter.GetPrefilteredSpecEnvMap();
		ID3D11ShaderResourceView* brdfLut = g_envMapFilter.GetBRDFLut();
		ID3D11ShaderResourceView* prefilteredDiffEnvMap = g_envMapFilter.GetPrefilteredDiffEnvMap();
		pd3dImmediateContext->PSSetShaderResources( SHADOW_MAP, 1, &g_shadowMap.srv );
		pd3dImmediateContext->PSSetShaderResources( ENVIRONMENT_MAP, 1, &environmentMap );
		pd3dImmediateContext->PSSetShaderResources( PREFILTERED_SPEC_ENV_MAP, 1, &prefilteredSpecEnvMap );
		pd3dImmediateContext->PSSetShaderResources( BRDF_LUT, 1, &brdfLut );
		pd3dImmediateContext->PSSetShaderResources( PREFILTERED_DIFF_ENV_MAP, 1, &prefilteredDiffEnvMap );
		pd3dImmediateContext->OMSetBlendState( g_doubleRtBlendState, nullptr, ~0u );

		if( GetGlobalControls().drawSky )
			g_skyRenderer.RenderLightPass( pd3dImmediateContext );
		if( GetGlobalControls().sceneType == SCENE_ONE_SPHERE || GetGlobalControls().sceneType == SCENE_MULTIPLE_SPHERES )
		{
			g_sphereRenderer.RenderLightPass( sphereInstances, &g_material, &g_merlMaterial, numSphereInstances, pd3dImmediateContext );
		}
		if( GetGlobalControls().sceneType == SCENE_SPONZA )
		{
			XMVECTOR pointLightFlux = XMVectorScale( GetSponzaSceneControls().pointLightColor, GetSponzaSceneControls().pointLightFlux );
			g_sponzaRenderer.RenderLightPass( pd3dImmediateContext, pointLightFlux, fTime );
		}

		if( GetGlobalControls().sceneType == SCENE_MULTIPLE_SPHERES )
			g_sphereRenderer.RenderPlaneLightPass( metalnessPlane, roughnessPlane, pd3dImmediateContext );

		DXUT_EndPerfEvent();
	}
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                  double fTime, float fElapsedTime, void* pUserContext )
{
	g_cachedGlobalParams.FrameIdx++;

	auto pRTV = DXUTGetD3D11RenderTargetView();
	auto pDSV = DXUTGetD3D11DepthStencilView();
	CBaseCamera* currentCamera = GetGlobalControls().sceneType == SCENE_ONE_SPHERE ? (CBaseCamera*)&g_modelViewerCamera : (CBaseCamera*)&g_firstPersonCamera;
	XMMATRIX viewProj = XMMatrixMultiply( currentCamera->GetViewMatrix(), currentCamera->GetProjMatrix() );

	if( GetGlobalControls().approxLevel == APPROX_LEVEL_IS || GetGlobalControls().approxLevel == APPROX_LEVEL_FIS )
	{
		if( memcmp( &viewProj, &g_lastFrameViewProj, sizeof( XMMATRIX ) ) != 0 )
			g_resetSampling = true;
	}

	if( g_cachedGlobalParams.SamplesProcessed == 0 || g_resetSampling ) 
	{
		float black[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pd3dImmediateContext->ClearRenderTargetView( g_indirectLightRenderTarget.rtv, black );
		g_lastFrameViewProj = viewProj;
		g_resetSampling = false;
		g_cachedGlobalParams.SamplesProcessed = 0;
	}

	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if( GetD3DSettingsDlg().IsActive() )
	{
		GetD3DSettingsDlg().OnRender( fElapsedTime );
		return;
	}

	pd3dImmediateContext->RSSetState( g_rasterizerState );
	pd3dImmediateContext->OMSetDepthStencilState( g_lightPassDepthStencilState, 0 );
	pd3dImmediateContext->OMSetBlendState( g_singleRtBlendState, nullptr, ~0u );
	pd3dImmediateContext->PSSetSamplers( LINEAR_CLAMP_SAMPLER_STATE, 1, &g_linearClampSamplerState );
	pd3dImmediateContext->CSSetSamplers( LINEAR_CLAMP_SAMPLER_STATE, 1, &g_linearClampSamplerState );
	pd3dImmediateContext->PSSetSamplers( CMP_LINEAR_SAMPLER_STATE, 1, &g_cmpLinearSamplerState );
	pd3dImmediateContext->PSSetSamplers( LINEAR_WRAP_SAMPLER_STATE, 1, &g_linearWrapSamplerState );
	pd3dImmediateContext->CSSetSamplers( LINEAR_WRAP_SAMPLER_STATE, 1, &g_linearWrapSamplerState );
	pd3dImmediateContext->VSSetConstantBuffers( GLOBAL_PARAMS_CB, 1, &g_globalParamsBuf );
	pd3dImmediateContext->PSSetConstantBuffers( GLOBAL_PARAMS_CB, 1, &g_globalParamsBuf );
	pd3dImmediateContext->CSSetConstantBuffers( GLOBAL_PARAMS_CB, 1, &g_globalParamsBuf );

	// update global params
	{
		g_cachedGlobalParams.ViewProjMatrix = viewProj;
		g_cachedGlobalParams.ViewPos = currentCamera->GetEyePt();
		float lightDirVert = ToRad( ( float )GetGlobalControls().lightDirVert );
		float lightDirHor = ToRad( ( float )GetGlobalControls().lightDirHor );
		g_cachedGlobalParams.LightDir = XMVectorSet( sin( lightDirVert ) * sin( lightDirHor ), cos( lightDirVert ), sin( lightDirVert ) * cos( lightDirHor ), 0.0f );
		g_cachedGlobalParams.LightIlluminance = XMVectorScale( GetGlobalControls().lightColor, GetGlobalControls().lightIlluminance);
		g_cachedGlobalParams.ShadowMatrix = XMMatrixIdentity();
		g_cachedGlobalParams.IndirectLightIntensity = GetGlobalControls().indirectLightIntensity;
		g_cachedGlobalParams.ApproxLevel = GetGlobalControls().approxLevel;
		g_cachedGlobalParams.TotalSamples = GetGlobalControls().samplesCount;
		g_cachedGlobalParams.SamplesInStep = GetGlobalControls().samplesPerFrame;
		g_cachedGlobalParams.EnableDirectLight = GetGlobalControls().enableDirectLight;
		g_cachedGlobalParams.EnableIndirectLight = GetGlobalControls().enableIndirectLight;
		g_cachedGlobalParams.EnableShadow = GetGlobalControls().enableShadow;
		g_cachedGlobalParams.EnableDiffuseLight = GetGlobalControls().enableDiffuseLight;
		g_cachedGlobalParams.EnableSpecularLight = GetGlobalControls().enableSpecularLight;
		FlushGlobalParams( pd3dImmediateContext );
	}

	BakeEnvMap( pd3dImmediateContext );
	if( GetGlobalControls().approxLevel == APPROX_LEVEL_BAKED_SPLIT_SUM_NV && g_cachedGlobalParams.SamplesProcessed == 0 )
		PrefilterEnvMap( pd3dImmediateContext );
	RenderScene( pd3dImmediateContext, ( float )fTime );	

	//
	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"Tonemap");
	ID3D11RenderTargetView* rtv[2];
	rtv[ 0 ] = pRTV;
	rtv[ 1 ] = nullptr;
	pd3dImmediateContext->OMSetRenderTargets( 2, rtv, pDSV );
	pd3dImmediateContext->ClearRenderTargetView( pRTV, Colors::MidnightBlue );
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );
	g_postProcess.Render( pd3dImmediateContext, g_directLightRenderTarget.srv, g_indirectLightRenderTarget.srv, GetGlobalControls().exposure );
	DXUT_EndPerfEvent();

	const uint32_t debugStrLen = 512;
	wchar_t debugStr[ debugStrLen ];
	memset( debugStr, 0, debugStrLen * sizeof( wchar_t ) );
	XMVECTOR p = currentCamera->GetEyePt();
	swprintf_s( debugStr, L"Camera position: %1.2f %1.2f %1.2f\nSamples processed: %d", p.m128_f32[ 0 ], p.m128_f32[ 1 ], p.m128_f32[ 2 ], g_cachedGlobalParams.SamplesProcessed );
	UIRender( pd3dDevice, pd3dImmediateContext, fElapsedTime, debugStr );

	if( GetGlobalControls().approxLevel == APPROX_LEVEL_IS || GetGlobalControls().approxLevel == APPROX_LEVEL_FIS )
	{
		if( g_cachedGlobalParams.SamplesProcessed < ( UINT )GetGlobalControls().samplesCount )
			g_cachedGlobalParams.SamplesProcessed += GetGlobalControls().samplesPerFrame;
	}
	else
	{
		g_cachedGlobalParams.SamplesProcessed = ( UINT )GetGlobalControls().samplesCount;
	}
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
	UIOnReleasingSwapChain();
	g_directLightRenderTarget.Release();
	g_indirectLightRenderTarget.Release();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	UIOnDestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();
	SAFE_RELEASE( g_globalParamsBuf );
	SAFE_RELEASE( g_rasterizerState );
	SAFE_RELEASE( g_depthPassDepthStencilState );
	SAFE_RELEASE( g_lightPassDepthStencilState );
	SAFE_RELEASE( g_singleRtBlendState );
	SAFE_RELEASE( g_doubleRtBlendState );
	SAFE_RELEASE( g_linearWrapSamplerState );
	SAFE_RELEASE( g_linearClampSamplerState );
	SAFE_RELEASE( g_cmpLinearSamplerState );
	g_directLightRenderTarget.Release();
	g_indirectLightRenderTarget.Release();
	g_shadowMap.Release();
	g_material.Release();
	g_merlMaterial.Release();

	g_sphereRenderer.OnD3D11DestroyDevice();
	g_skyRenderer.OnD3D11DestroyDevice();
	g_sponzaRenderer.OnD3D11DestroyDevice();
	g_postProcess.OnD3D11DestroyDevice();
	g_envMapFilter.OnD3D11DestroyDevice();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
	if( !UIMsgProc( hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing, pUserContext ) )
		return 0;

	// Pass all remaining windows messages to camera so it can respond to user input
	if( GetGlobalControls().sceneType == SCENE_ONE_SPHERE )
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


void OnShaderReloadCallback()
{
	g_skyRenderer.ReloadShaders( DXUTGetD3D11Device() );
	g_sphereRenderer.ReloadShaders( DXUTGetD3D11Device() );
	g_sponzaRenderer.ReloadShaders( DXUTGetD3D11Device() );
	g_postProcess.ReloadShaders( DXUTGetD3D11Device() );
	g_envMapFilter.ReloadShaders( DXUTGetD3D11Device() );
	g_resetSampling = true;
}



void OnResetSamplingCallback()
{
	g_resetSampling = true;
}


void OnMaterialChangeCallback()
{
	if( GetOneSphereSceneControls().materialType == MATERIAL_TEXTURE )
		g_material.Load( DXUTGetD3D11Device(), GetOneSphereSceneControls().textureMaterial );
	if( GetOneSphereSceneControls().materialType == MATERIAL_MERL )
		g_merlMaterial.Load( DXUTGetD3D11Device(), GetOneSphereSceneControls().merlMaterial );
	g_resetSampling = true;
}


void OnSkyTextureChangeCallback()
{
	g_skyRenderer.LoadSkyTexture( DXUTGetD3D11Device(), GetGlobalControls().skyTexture );
	g_resetSampling = true;
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
	UIInit();
	SetOnShaderReloadCallback( &OnShaderReloadCallback );
	SetOnResetSamplingCallback( &OnResetSamplingCallback );
	SetOnMaterialChangeCallback( &OnMaterialChangeCallback );
	SetOnSkyTextureChangeCallback( &OnSkyTextureChangeCallback );

    DXUTInit( true, true, nullptr ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"PBR" );

    // 
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1920, 1080 );
    DXUTMainLoop(); // Enter into the DXUT ren  der loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}
