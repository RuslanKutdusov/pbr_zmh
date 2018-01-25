#include "Precompiled.h"


using namespace DirectX;


namespace
{
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
		IDC_LIGHT_IRRADIANCE_STATIC,
		IDC_LIGHT_IRRADIANCE,
		IDC_LIGHT_COLOR,
		IDC_METALNESS_STATIC,
		IDC_METALNESS,
		IDC_ROUGHNESS_STATIC,
		IDC_ROUGHNESS,
		IDC_ALBEDO,
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

	GlobalControls				g_globalControls;
	OneSphereSceneControls		g_oneSphereSceneControls;
	MultipleSphereSceneControls g_multipleSphereSceneControls;

	CDXUTDialogResourceManager	g_DialogResourceManager; // manager for shared resources of dialogs
	CD3DSettingsDlg				g_D3DSettingsDlg;       // Device settings dialog
	CDXUTDialog                 g_HUD;                  // manages the 3D UI
	CDXUTTextHelper*            g_pTxtHelper = nullptr;

	UICallback					g_onShaderReload = nullptr;
	UICallback					g_onResetSampling = nullptr;
	UICallback					g_onMaterialChangeCallback = nullptr;
	UICallback					g_onSkyTextureChangeCallback = nullptr;


	void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
	{
		WCHAR str[ MAX_PATH ];

		switch( nControlID )
		{
			case IDC_CHANGEDEVICE:
			{
				g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() );
				break;
			}
			case IDC_RELOAD_SHADERS:
			{
				if( g_onShaderReload )
					g_onShaderReload();
				break;
			}
			case IDC_LIGHTVERT:
			{
				g_globalControls.lightDirVert = g_HUD.GetSlider( IDC_LIGHTVERT )->GetValue();
				swprintf_s( str, MAX_PATH, L"Light vert: %d", g_globalControls.lightDirVert );
				g_HUD.GetStatic( IDC_LIGHTVERT_STATIC )->SetText( str );
				break;
			}
			case IDC_LIGHTHOR:
			{
				g_globalControls.lightDirHor = g_HUD.GetSlider( IDC_LIGHTHOR )->GetValue();
				swprintf_s( str, MAX_PATH, L"Light hor: %d", g_globalControls.lightDirHor );
				g_HUD.GetStatic( IDC_LIGHTHOR_STATIC )->SetText( str );
				break;
			}
			case IDC_LIGHT_IRRADIANCE:
			{
				g_globalControls.lightIrradiance = g_HUD.GetSlider( IDC_LIGHT_IRRADIANCE )->GetValue() / 5.0f;
				swprintf_s( str, MAX_PATH, L"Light irradiance( W/m2 ): %1.2f", g_globalControls.lightIrradiance );
				g_HUD.GetStatic( IDC_LIGHT_IRRADIANCE_STATIC )->SetText( str );
				break;
			}
			case IDC_LIGHT_COLOR:
			{
				CHOOSECOLOR cc;
				COLORREF acrCustClr[ 16 ];
				memset( &cc, 0, sizeof( cc ) );
				cc.lStructSize = sizeof( cc );
				cc.lpCustColors = ( LPDWORD )acrCustClr;
				cc.rgbResult = VectorToColor( g_globalControls.lightColor );
				cc.Flags = CC_FULLOPEN | CC_RGBINIT;

				if( ChooseColor( &cc ) == TRUE )
					g_globalControls.lightColor = ColorToVector( cc.rgbResult );

				break;
			}
			case IDC_METALNESS:
			{
				g_oneSphereSceneControls.metalness = g_HUD.GetSlider( IDC_METALNESS )->GetValue() / 100.0f;
				swprintf_s( str, MAX_PATH, L"Metalness: %1.2f", g_oneSphereSceneControls.metalness );
				g_HUD.GetStatic( IDC_METALNESS_STATIC )->SetText( str );
				if( g_onResetSampling )
					g_onResetSampling();
				break;
			}
			case IDC_ROUGHNESS:
			{
				g_oneSphereSceneControls.roughness = g_HUD.GetSlider( IDC_ROUGHNESS )->GetValue() / 100.0f;
				swprintf_s( str, MAX_PATH, L"Roughness: %1.2f", g_oneSphereSceneControls.roughness );
				g_HUD.GetStatic( IDC_ROUGHNESS_STATIC )->SetText( str );
				if( g_onResetSampling )
					g_onResetSampling();
				break;
			}
			case IDC_ALBEDO:
			{
				CHOOSECOLOR cc;
				COLORREF acrCustClr[ 16 ];
				memset( &cc, 0, sizeof( cc ) );
				cc.lStructSize = sizeof( cc );
				cc.lpCustColors = ( LPDWORD )acrCustClr;
				cc.rgbResult = VectorToColor( g_oneSphereSceneControls.albedo );
				cc.Flags = CC_FULLOPEN | CC_RGBINIT;

				if( ChooseColor( &cc ) == TRUE )
				{
					g_oneSphereSceneControls.albedo = ColorToVector( cc.rgbResult );
					if( g_onResetSampling )
						g_onResetSampling();
				}

				break;
			}
			case IDC_USE_MATERIAL:
			{
				g_oneSphereSceneControls.useMaterial = g_HUD.GetCheckBox( IDC_USE_MATERIAL )->GetChecked();
				if( g_onResetSampling )
					g_onResetSampling();
				break;
			}
			case IDC_MATERIAL:
			{
				g_oneSphereSceneControls.material = ( const wchar_t* )g_HUD.GetComboBox( IDC_MATERIAL )->GetSelectedData();
				if( g_onMaterialChangeCallback )
					g_onMaterialChangeCallback();
			}
			case IDC_DIRECT_LIGHT:
			{
				g_globalControls.enableDirectLight = g_HUD.GetCheckBox( IDC_DIRECT_LIGHT )->GetChecked();
				break;
			}
			case IDC_INDIRECT_LIGHT:
			{
				g_globalControls.enableIndirectLight = g_HUD.GetCheckBox( IDC_INDIRECT_LIGHT )->GetChecked();
				if( g_onResetSampling )
					g_onResetSampling();
				break;
			}
			case IDC_EXPOSURE:
			{
				g_globalControls.exposure = g_HUD.GetSlider( IDC_EXPOSURE )->GetValue() / 20.0f;
				swprintf_s( str, MAX_PATH, L"Exposure: %1.2f", g_globalControls.exposure );
				g_HUD.GetStatic( IDC_EXPOSURE_STATIC )->SetText( str );
				break;
			}
			case IDC_DRAW_SKY:
			{
				g_globalControls.drawSky = g_HUD.GetCheckBox( IDC_DRAW_SKY )->GetChecked();
				break;
			}
			case IDC_SELECT_SKY_TEXTURE:
			{
				g_globalControls.skyTexture = ( const wchar_t* )g_HUD.GetComboBox( IDC_SELECT_SKY_TEXTURE )->GetSelectedData();
				if( g_onSkyTextureChangeCallback )
					g_onSkyTextureChangeCallback();
			}
			case IDC_SCENE_TYPE:
			{
				g_globalControls.sceneType = ( SCENE_TYPE )PtrToUlong( g_HUD.GetComboBox( IDC_SCENE_TYPE )->GetSelectedData() );
				if( g_onResetSampling )
					g_onResetSampling();
				break;
			}
		}
	}
}


void UIInit()
{
	g_D3DSettingsDlg.Init( &g_DialogResourceManager );
	g_HUD.Init( &g_DialogResourceManager );
	g_HUD.SetCallback( OnGUIEvent );

	int iY = 10;
	g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY, HUD_WIDTH, 23, VK_F2 );
	g_HUD.AddButton( IDC_RELOAD_SHADERS, L"Reload shaders (F3)", 0, iY += 25, HUD_WIDTH, 23, VK_F3 );

	iY += 50;
	WCHAR str[ MAX_PATH ];
	swprintf_s( str, MAX_PATH, L"Light vert: %d", g_globalControls.lightDirVert );
	g_HUD.AddStatic( IDC_LIGHTVERT_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_LIGHTVERT, 0, iY += 24, HUD_WIDTH, 22, 0, 180, g_globalControls.lightDirVert );

	swprintf_s( str, MAX_PATH, L"Light hor: %d", g_globalControls.lightDirHor );
	g_HUD.AddStatic( IDC_LIGHTHOR_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_LIGHTHOR, 0, iY += 24, HUD_WIDTH, 22, 0, 180, g_globalControls.lightDirHor );

	swprintf_s( str, MAX_PATH, L"Light irradiance( W/m2 ): %1.2f", g_globalControls.lightIrradiance );
	g_HUD.AddStatic( IDC_LIGHT_IRRADIANCE_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_LIGHT_IRRADIANCE, 0, iY += 24, HUD_WIDTH, 22, 0, 100, ( int )( g_globalControls.lightIrradiance * 5.0f ) );

	g_HUD.AddButton( IDC_LIGHT_COLOR, L"Light color", 0, iY += 25, HUD_WIDTH, 23 );

	swprintf_s( str, MAX_PATH, L"Exposure: %1.2f", g_globalControls.exposure );
	g_HUD.AddStatic( IDC_EXPOSURE_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_EXPOSURE, 0, iY += 24, HUD_WIDTH, 22, 0, 100, ( int )( g_globalControls.exposure * 20.0f ) );

	swprintf_s( str, MAX_PATH, L"Draw sky" );
	g_HUD.AddCheckBox( IDC_DRAW_SKY, str, 0, iY += 24, HUD_WIDTH, 22, g_globalControls.drawSky );

	CDXUTComboBox* skyTextureComboxBox;
	g_HUD.AddComboBox( IDC_SELECT_SKY_TEXTURE, 0, iY += 24, HUD_WIDTH, 22, 0, false, &skyTextureComboxBox );
	for( size_t i = 0; i < sizeof( g_skyTextureFiles ) / sizeof( wchar_t* ); i++ )
		skyTextureComboxBox->AddItem( g_skyTextureFiles[ i ], ( void* )g_skyTextureFiles[ i ] );

	swprintf_s( str, MAX_PATH, L"Enable direct light" );
	g_HUD.AddCheckBox( IDC_DIRECT_LIGHT, str, 0, iY += 24, HUD_WIDTH, 22, g_globalControls.enableDirectLight );

	swprintf_s( str, MAX_PATH, L"Enable indirect light" );
	g_HUD.AddCheckBox( IDC_INDIRECT_LIGHT, str, 0, iY += 24, HUD_WIDTH, 22, g_globalControls.enableIndirectLight );

	CDXUTComboBox* sceneComboBox;
	g_HUD.AddComboBox( IDC_SCENE_TYPE, 0, iY += 24, HUD_WIDTH, 22, 0, false, &sceneComboBox );
	sceneComboBox->AddItem( L"One sphere", ULongToPtr( SCENE_ONE_SPHERE ) );
	sceneComboBox->AddItem( L"Multiple spheres", ULongToPtr( SCENE_MULTIPLE_SPHERES ) );
	//sceneComboBox->AddItem( L"Sponza", ULongToPtr( SCENE_SPONZA ) );

	swprintf_s( str, MAX_PATH, L"Metalness: %1.2f", g_oneSphereSceneControls.metalness );
	g_HUD.AddStatic( IDC_METALNESS_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_METALNESS, 0, iY += 24, HUD_WIDTH, 22, 0, 100, ( int )( g_oneSphereSceneControls.metalness * 100.0f ) );

	swprintf_s( str, MAX_PATH, L"Roughness: %1.2f", g_oneSphereSceneControls.roughness );
	g_HUD.AddStatic( IDC_ROUGHNESS_STATIC, str, 0, iY += 24, HUD_WIDTH, 22 );
	g_HUD.AddSlider( IDC_ROUGHNESS, 0, iY += 24, HUD_WIDTH, 22, 0, 100, ( int )( g_oneSphereSceneControls.roughness * 100.0f ) );

	g_HUD.AddButton( IDC_ALBEDO, L"Albedo", 0, iY += 25, HUD_WIDTH, 23 );

	swprintf_s( str, MAX_PATH, L"Use material" );
	g_HUD.AddCheckBox( IDC_USE_MATERIAL, str, 0, iY += 24, HUD_WIDTH, 22, g_oneSphereSceneControls.useMaterial );

	CDXUTComboBox* materialComboxBox;
	g_HUD.AddComboBox( IDC_MATERIAL, 0, iY += 24, HUD_WIDTH, 22, 0, false, &materialComboxBox );
	for( size_t i = 0; i < sizeof( g_materials ) / sizeof( wchar_t* ); i++ )
		materialComboxBox->AddItem( g_materials[ i ], ( void* )g_materials[ i ] );

	g_globalControls.skyTexture = g_skyTextureFiles[ 0 ];
}


HRESULT UIOnDeviceCreate( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext )
{
	HRESULT hr = S_OK;

	V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
	V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
	g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

	return S_OK;
}


HRESULT UIOnResizedSwapChain( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc )
{
	HRESULT hr = S_OK;

	V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
	V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

	g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - HUD_WIDTH - 10, 0 );
	g_HUD.SetSize( HUD_WIDTH, 170 );

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

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
	if( *pbNoFurtherProcessing )
		return false;

	return true;
}


void UIRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime )
{
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
	g_HUD.OnRender( fElapsedTime );
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos( 2, 0 );
	g_pTxtHelper->SetForegroundColor( Colors::Yellow );
	g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
	g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
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