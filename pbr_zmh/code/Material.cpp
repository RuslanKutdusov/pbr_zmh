#include "Precompiled.h"


HRESULT Material::Load( ID3D11Device* pd3dDevice, const wchar_t* materialName )
{
	const wchar_t* extension = L"dds";
	const UINT printfBufSize = 256;
	wchar_t albedoPath[ printfBufSize ];
	wchar_t normalPath[ printfBufSize ];
	wchar_t roughnessPath[ printfBufSize ];
	wchar_t metalnessPath[ printfBufSize ];

	memset( albedoPath, 0, printfBufSize * sizeof( wchar_t ) );
	wsprintf( albedoPath, L"%s\\albedo.%s", materialName, extension );

	memset( normalPath, 0, printfBufSize * sizeof( wchar_t ) );
	wsprintf( normalPath, L"%s\\normal.%s", materialName, extension );

	memset( roughnessPath, 0, printfBufSize * sizeof( wchar_t ) );
	wsprintf( roughnessPath, L"%s\\roughness.%s", materialName, extension );

	memset( metalnessPath, 0, printfBufSize * sizeof( wchar_t ) );
	wsprintf( metalnessPath, L"%s\\metalness.%s", materialName, extension );

	return Load( pd3dDevice, materialName, albedoPath, normalPath, roughnessPath, metalnessPath );
}


HRESULT Material::Load( ID3D11Device* pd3dDevice, const wchar_t* materialName, const wchar_t* albedoPath, const wchar_t* normalPath,
	const wchar_t* roughnessPath, const wchar_t* metalnessPath )
{
	ID3D11DeviceContext* ctx;
	pd3dDevice->GetImmediateContext( &ctx );
	HRESULT hr;
	Release();

	hr = DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx, albedoPath, &albedo, true );
	if( FAILED( hr ) )
	{
		DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx,
			L"materials\\default\\albedo.dds", &albedo, true );
	}

	hr = DXUTCreateShaderResourceViewFromFile( pd3dDevice, normalPath, &normal );
	if( FAILED( hr ) )
	{
		DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx,
			L"materials\\default\\normal.dds", &normal, false );
	}

	hr = DXUTCreateShaderResourceViewFromFile( pd3dDevice, roughnessPath, &roughness );
	if( FAILED( hr ) )
	{
		DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx,
			L"materials\\default\\roughness.dds", &roughness, false );
	}

	hr = DXUTCreateShaderResourceViewFromFile( pd3dDevice, metalnessPath, &metalness );
	if( FAILED( hr ) )
	{
		DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx,
			L"materials\\default\\metalness.dds", &metalness, false );
	}

	name = materialName;
	SAFE_RELEASE( ctx );
	return S_OK;
}


void Material::Release()
{
	SAFE_RELEASE( albedo );
	SAFE_RELEASE( normal );
	SAFE_RELEASE( roughness );
	SAFE_RELEASE( metalness );
	name.clear();
}