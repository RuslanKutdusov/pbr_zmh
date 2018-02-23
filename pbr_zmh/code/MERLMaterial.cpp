#include "Precompiled.h"

#define BRDF_SAMPLING_RES_THETA_H       90
#define BRDF_SAMPLING_RES_THETA_D       90
#define BRDF_SAMPLING_RES_PHI_D         360

HRESULT MERLMaterial::Load( ID3D11Device* pd3dDevice, const char* materialName )
{
	SAFE_RELEASE( m_brdfSRV );
	SAFE_RELEASE( m_brdf );

	wchar_t relPath[ MAX_PATH ];
	wsprintf( relPath, L"%S", materialName );
	wchar_t fullPath[ MAX_PATH ];
	if( FAILED( DXUTFindDXSDKMediaFileCch( fullPath, MAX_PATH, relPath ) ) )
	{
		ErrorMessageBox( "Failed to find MERL BRDF" );
		return E_FAIL;
	}

	FILE *f = nullptr;
	_wfopen_s( &f, fullPath, L"rb" );
	if( !f )
		return false;

	int dims[ 3 ];
	fread( dims, sizeof( int ), 3, f );
	int n = dims[ 0 ] * dims[ 1 ] * dims[ 2 ];
	if( n != BRDF_SAMPLING_RES_THETA_H *
		BRDF_SAMPLING_RES_THETA_D *
		BRDF_SAMPLING_RES_PHI_D / 2 )
	{
		fprintf( stderr, "Dimensions don't match\n" );
		fclose( f );
		return false;
	}

	double* brdfDouble = ( double* )malloc( sizeof( double ) * 3 * n );
	fread( brdfDouble, sizeof( double ), 3 * n, f );
	fclose( f );

	float* brdfFloat = ( float* )malloc( sizeof( float ) * 3 * n );
	for( int i = 0; i < 3 * n; i++ )
		brdfFloat[ i ] = ( float )brdfDouble[ i ];

	D3D11_BUFFER_DESC bufDesc;
	ZeroMemory( &bufDesc, sizeof( bufDesc ) );
	bufDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bufDesc.ByteWidth = sizeof( float ) * 3 * n;
	bufDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = brdfFloat;
	data.SysMemPitch = bufDesc.ByteWidth;
	data.SysMemSlicePitch = 0;

	HRESULT hr;
	V_RETURN( pd3dDevice->CreateBuffer( &bufDesc, &data, &m_brdf ) );

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory( &srvDesc, sizeof( srvDesc ) );
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = 3 * n;
	V_RETURN( pd3dDevice->CreateShaderResourceView( m_brdf, &srvDesc, &m_brdfSRV ) );

	free( brdfDouble );
	free( brdfFloat );

	return S_OK;
}


void MERLMaterial::Release()
{
	SAFE_RELEASE( m_brdfSRV );
	SAFE_RELEASE( m_brdf );
}