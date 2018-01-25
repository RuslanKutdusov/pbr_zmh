#include "Precompiled.h"


HRESULT Material::Load( ID3D11Device* pd3dDevice, const wchar_t* materialName )
{
	ID3D11DeviceContext* ctx;
	pd3dDevice->GetImmediateContext( &ctx );
	HRESULT hr;
	Release();

	const wchar_t* extension = L"dds";
	const UINT printfBufSize = 256;
	wchar_t printfBuf[ printfBufSize ];

	memset( printfBuf, 0, printfBufSize * sizeof( wchar_t ) );
	wsprintf( printfBuf, L"%s\\albedo.%s", materialName, extension );
	hr = DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx, printfBuf, &albedo, true );
	if( FAILED( hr ) )
	{
		DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx, 
			L"materials\\default\\albedo.dds", &albedo, true );
	}

	memset( printfBuf, 0, printfBufSize * sizeof( wchar_t ) );
	wsprintf( printfBuf, L"%s\\normal.%s", materialName, extension );
	hr = DXUTCreateShaderResourceViewFromFile( pd3dDevice, printfBuf, &normal );
	if( FAILED( hr ) )
	{
		DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx, 
			L"materials\\default\\normal.dds", &normal, false );
	}

	memset( printfBuf, 0, printfBufSize * sizeof( wchar_t ) );
	wsprintf( printfBuf, L"%s\\roughness.%s", materialName, extension );
	hr = DXUTCreateShaderResourceViewFromFile( pd3dDevice, printfBuf, &roughness );
	if( FAILED( hr ) )
	{
		DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, ctx, 
			L"materials\\default\\roughness.dds", &roughness, false );
	}

	memset( printfBuf, 0, printfBufSize * sizeof( wchar_t ) );
	wsprintf( printfBuf, L"%s\\metalness.%s", materialName, extension );
	hr = DXUTCreateShaderResourceViewFromFile( pd3dDevice, printfBuf, &metalness );
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