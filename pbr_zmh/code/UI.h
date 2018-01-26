#pragma once

enum SCENE_TYPE
{
	SCENE_ONE_SPHERE = 0,
	SCENE_MULTIPLE_SPHERES,
	SCENE_SPONZA,

	SCENE_TYPE_COUNT
};


struct GlobalControls
{
	int lightDirVert			= 45;
	int lightDirHor				= 130;
	float lightIrradiance		= 1.0f;
	DirectX::XMVECTOR lightColor = DirectX::XMVectorSet( 1.0f, 1.0f, 1.0f, 0.0f );
	float indirectLightIntensity = 1.0f;
	bool enableDirectLight		= true;
	bool enableIndirectLight	= true;
	bool enableShadow			= true;
	bool enableDiffuseLight		= true;
	bool enableSpecularLight	= true;
	float exposure				= 1.0f;
	bool drawSky				= true;
	const wchar_t* skyTexture	= L"";
	SCENE_TYPE sceneType		= SCENE_ONE_SPHERE;
};


struct OneSphereSceneControls
{
	float metalness				= 1.0f;
	float roughness				= 0.5f;
	float reflectance			= 1.0f;
	DirectX::XMVECTOR albedo	= DirectX::XMVectorSet( 1.0f, 1.0f, 1.0f, 0.0f );
	bool useMaterial			= false;
	const wchar_t* material		= L"materials\\default";
};


struct MultipleSphereSceneControls
{
	DirectX::XMVECTOR albedo = DirectX::XMVectorSet( 1.0f, 1.0f, 1.0f, 0.0f );
};


void UIInit();
HRESULT UIOnDeviceCreate( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext );
HRESULT UIOnResizedSwapChain( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc );
void UIOnReleasingSwapChain();
void UIOnDestroyDevice();

bool UIMsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void UIRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime, const wchar_t* debugStr );

CD3DSettingsDlg& GetD3DSettingsDlg();
const GlobalControls& GetGlobalControls();
const OneSphereSceneControls& GetOneSphereSceneControls();
const MultipleSphereSceneControls& GetMultipleSphereSceneControls();

using UICallback = void(*)();
void SetOnShaderReloadCallback( UICallback callback );
void SetOnResetSamplingCallback( UICallback callback );
void SetOnMaterialChangeCallback( UICallback callback );
void SetOnSkyTextureChangeCallback( UICallback callback );
