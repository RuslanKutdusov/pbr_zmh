#include "Precompiled.h"
#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx11.h"

using namespace DirectX;


namespace
{
	const char* g_skyTextureFiles[] = {
		"HDRs\\galileo_cross.dds",
		"HDRs\\grace_cross.dds",
		"HDRs\\rnl_cross.dds",
		"HDRs\\stpeters_cross.dds",
		"HDRs\\uffizi_cross.dds",
		"HDRs\\ennis.dds",
		"HDRs\\grace-new.dds",
		"HDRs\\pisa.dds",
	};

	const char* g_materials[] = {
		"materials\\default",
		"materials\\Brick_baked",
		"materials\\Brick_Beige",
		"materials\\Brick_Cinder",
		"materials\\Brick_Concrete",
		"materials\\Brick_Yellow",
		"materials\\Concrete_Shimizu",
		"materials\\Concrete_Tadao",
		"materials\\copper-oxidized",
		"materials\\copper-rock",
		"materials\\copper-scuffed",
		"materials\\greasy-pan",
		"materials\\OldPlaster_00",
		"materials\\OldPlaster_01",
		"materials\\rustediron",
		"materials\\streakedmetal",
		"materials\\T_Paint_Black",
		"materials\\T_Tile_White",
	};

	GlobalControls				g_globalControls;
	OneSphereSceneControls		g_oneSphereSceneControls;
	MultipleSphereSceneControls g_multipleSphereSceneControls;
	SponzaSceneControls			g_sponzaSceneControls;

	CDXUTDialogResourceManager	g_DialogResourceManager; // manager for shared resources of dialogs
	CD3DSettingsDlg				g_D3DSettingsDlg;       // Device settings dialog
	CDXUTTextHelper*            g_pTxtHelper = nullptr;

	UICallback					g_onShaderReload = nullptr;
	UICallback					g_onResetSampling = nullptr;
	UICallback					g_onMaterialChangeCallback = nullptr;
	UICallback					g_onSkyTextureChangeCallback = nullptr;


	bool ChooseColor( XMVECTOR& color )
	{
		CHOOSECOLOR cc;
		COLORREF acrCustClr[ 16 ];
		memset( &cc, 0, sizeof( cc ) );
		cc.lStructSize = sizeof( cc );
		cc.lpCustColors = ( LPDWORD )acrCustClr;
		cc.rgbResult = VectorToColor( color );
		cc.Flags = CC_FULLOPEN | CC_RGBINIT;

		if( ChooseColor( &cc ) == TRUE )
		{
			color = ColorToVector( cc.rgbResult );
			return true;
		}

		return false;
	}
}


void UIInit()
{
	g_D3DSettingsDlg.Init( &g_DialogResourceManager );
	g_globalControls.skyTexture = g_skyTextureFiles[ 0 ];
}


HRESULT UIOnDeviceCreate( HWND hwnd, ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext )
{
	HRESULT hr = S_OK;

	V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
	V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
	g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = 1920.0f;
	io.DisplaySize.y = 1280.0f;
	ImGui_ImplDX11_Init( hwnd, pd3dDevice, pd3dImmediateContext );

	return S_OK;
}


HRESULT UIOnResizedSwapChain( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc )
{
	HRESULT hr = S_OK;

	V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
	V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

	return S_OK;
}


void UIOnReleasingSwapChain()
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


void UIOnDestroyDevice()
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
	g_D3DSettingsDlg.OnD3D11DestroyDevice();
	SAFE_DELETE( g_pTxtHelper );
	ImGui_ImplDX11_Shutdown();
	ImGui::DestroyContext();
}


bool UIMsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
	if( *pbNoFurtherProcessing )
		return false;

	// Pass messages to settings dialog if its active
	if( g_D3DSettingsDlg.IsActive() )
	{
		g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
		return false;
	}

	ImGui_ImplWin32_WndProcHandler( hWnd, uMsg, wParam, lParam );
	if( ImGui::GetCurrentContext() )
	{
		ImGuiIO& io = ImGui::GetIO();
		if( io.WantCaptureMouse || io.WantCaptureKeyboard )
			return false;
	}

	return true;
}


void UIRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime, const wchar_t* debugStr )
{
	ImGui_ImplDX11_NewFrame();

	static bool my_tool_active = true;
	ImGui::Begin( "Controls", &my_tool_active, ImGuiWindowFlags_NoTitleBar );
	if( ImGui::CollapsingHeader( "Global controls", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		if( ImGui::Button( "Change device" ) )
			g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() );
		if( ImGui::Button( "Reload shaders" ) )
		{
			if( g_onShaderReload )
				g_onShaderReload();
		}
		ImGui::SliderFloat( "Light vertical angle", &g_globalControls.lightDirVert, 0.0f, 180.0f );
		ImGui::SliderFloat( "Light horizontal angle", &g_globalControls.lightDirHor, 0.0f, 180.0f );
		ImGui::SliderFloat( "Light irradiance( W/m2 )", &g_globalControls.lightIrradiance, 0.0f, 20.0f );
		if( ImGui::Button( "Light color" ) )
			ChooseColor( g_globalControls.lightColor );
		if( ImGui::SliderFloat( "Indirect light intensity", &g_globalControls.indirectLightIntensity, 0.0f, 20.0f ) )
			g_onResetSampling();
		ImGui::SliderInt( "Approximation level", &g_globalControls.approxLevel, 0, 2 );
		ImGui::SliderFloat( "Exposure", &g_globalControls.exposure, 0.0f, 5.0f );
		ImGui::Checkbox( "Draw sky", &g_globalControls.drawSky );
		{
			if( ImGui::BeginCombo( "Sky texture", g_globalControls.skyTexture ) ) // The second parameter is the label previewed before opening the combo.
			{
				for( int n = 0; n < IM_ARRAYSIZE( g_skyTextureFiles ); n++ )
				{
					bool is_selected = ( g_globalControls.skyTexture == g_skyTextureFiles[ n ] ); // You can store your selection however you want, outside or inside your objects
					if( ImGui::Selectable( g_skyTextureFiles[ n ], is_selected ) )
					{
						g_globalControls.skyTexture = g_skyTextureFiles[ n ];
						g_onSkyTextureChangeCallback();
					}
					if( is_selected )
						ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
				}
				ImGui::EndCombo();
			}
		}
		ImGui::Checkbox( "Enable direct light", &g_globalControls.enableDirectLight );
		if( ImGui::Checkbox( "Enable indirect light", &g_globalControls.enableIndirectLight ) )
			g_onResetSampling();
		if( ImGui::Checkbox( "Enable diffuse light", &g_globalControls.enableDiffuseLight ) )
			g_onResetSampling();
		if( ImGui::Checkbox( "Enable specular light", &g_globalControls.enableSpecularLight ) )
			g_onResetSampling();
		ImGui::Checkbox( "Enable shadow", &g_globalControls.enableShadow );
	}

	if( ImGui::CollapsingHeader( "Scene controls", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		if( ImGui::Combo( "Scene", ( int * )&g_globalControls.sceneType, "One sphere\0Multiple spheres\0Sponza\0\0" ) )
			g_onResetSampling();

		if( g_globalControls.sceneType == SCENE_ONE_SPHERE )
		{
			if( ImGui::Checkbox( "Use Material", &g_oneSphereSceneControls.useMaterial ) )
				g_onResetSampling();

			if( g_oneSphereSceneControls.useMaterial )
			{
				if( ImGui::BeginCombo( "Material", g_oneSphereSceneControls.material ) ) // The second parameter is the label previewed before opening the combo.
				{
					for( int n = 0; n < IM_ARRAYSIZE( g_materials ); n++ )
					{
						bool is_selected = ( g_oneSphereSceneControls.material == g_materials[ n ] ); // You can store your selection however you want, outside or inside your objects
						if( ImGui::Selectable( g_materials[ n ], is_selected ) )
						{
							g_oneSphereSceneControls.material = g_materials[ n ];
							g_onMaterialChangeCallback();
						}
						if( is_selected )
							ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
					}
					ImGui::EndCombo();
				}
			}
			else
			{
				if( ImGui::SliderFloat( "Metalness", &g_oneSphereSceneControls.metalness, 0.0f, 1.0f ) )
					g_onResetSampling();
				if( ImGui::SliderFloat( "Roughness", &g_oneSphereSceneControls.roughness, 0.0f, 1.0f ) )
					g_onResetSampling();
				if( ImGui::SliderFloat( "Reflectance", &g_oneSphereSceneControls.reflectance, 0.0f, 1.0f ) )
					g_onResetSampling();
				if( ImGui::Button( "Albedo" ) )
				{
					if( ChooseColor( g_oneSphereSceneControls.albedo ) )
						g_onResetSampling();
				}
			}
		}

		if( g_globalControls.sceneType == SCENE_MULTIPLE_SPHERES )
		{
			if( ImGui::Button( "Albedo" ) )
			{
				if( ChooseColor( g_multipleSphereSceneControls.albedo ) )
					g_onResetSampling();
			}
		}

		if( g_globalControls.sceneType == SCENE_SPONZA )
		{
			ImGui::SliderFloat( "Point light flux( Watt )", &g_sponzaSceneControls.pointLightFlux, 0.0f, 5.0f );
			if( ImGui::Button( "Point light color" ) )
				ChooseColor( g_sponzaSceneControls.pointLightColor );
		}
	}
	ImGui::End();
	ImGui::Render();

	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );

	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos( 2, 0 );
	g_pTxtHelper->SetForegroundColor( Colors::Yellow );
	g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
	g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
	if( debugStr )
		g_pTxtHelper->DrawTextLine( debugStr );
	g_pTxtHelper->End();
	DXUT_EndPerfEvent();
}


CD3DSettingsDlg& GetD3DSettingsDlg()
{
	return g_D3DSettingsDlg;
}


const GlobalControls& GetGlobalControls()
{
	return g_globalControls;
}


const OneSphereSceneControls& GetOneSphereSceneControls()
{
	return g_oneSphereSceneControls;
}


const MultipleSphereSceneControls& GetMultipleSphereSceneControls()
{
	return g_multipleSphereSceneControls;
}


const SponzaSceneControls& GetSponzaSceneControls()
{
	return g_sponzaSceneControls;
}


void SetOnShaderReloadCallback( UICallback callback )
{
	g_onShaderReload = callback;
}


void SetOnResetSamplingCallback( UICallback callback )
{
	g_onResetSampling = callback;
}


void SetOnMaterialChangeCallback( UICallback callback )
{
	g_onMaterialChangeCallback = callback;
}


void SetOnSkyTextureChangeCallback( UICallback callback )
{
	g_onSkyTextureChangeCallback = callback;
}