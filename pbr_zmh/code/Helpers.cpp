#include "Precompiled.h"
using namespace DirectX;


void ErrorMessageBox( const char* text, ... )
{
	char buffer[ 1024 ];
	memset( buffer, 0, 1024 );
	va_list args;
	va_start( args, text );
	vsprintf_s( buffer, text, args );
	va_end( args );

	MessageBoxA( 0, buffer, "Error", MB_OK );
}


HRESULT CompileShader( LPCWSTR path, const D3D_SHADER_MACRO* pDefines, LPCSTR pEntrypoint, SHADER_TYPE shaderType, ID3DBlob** bytecodeBlob )
{
	WCHAR fullPath[ MAX_PATH ];
	if( FAILED( DXUTFindDXSDKMediaFileCch( fullPath, MAX_PATH, path ) ) )
	{
		ErrorMessageBox( "Shader file does not exists:\n%ws", fullPath );
		return E_FAIL;
	}

	LPCSTR target = "";
	switch( shaderType )
	{
		case SHADER_VERTEX:
			target = "vs_5_0";
			break;
		case SHADER_GEOMETRY:
			target = "gs_5_0";
			break;
		case SHADER_PIXEL:
			target = "ps_5_0";
			break;
		case SHADER_COMPUTE:
			target = "cs_5_0";
			break;
		default:
			break;
	}


	ID3DBlob* errorBlob = nullptr;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;
	HRESULT hr = D3DCompileFromFile( fullPath, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
									pEntrypoint, target, flags, 0, bytecodeBlob, &errorBlob );
	if( FAILED( hr ) )
	{
		if( errorBlob )
			ErrorMessageBox( "Failed to compile shader:\n%s", ( const char* )errorBlob->GetBufferPointer() );
		else
			ErrorMessageBox( "Failed to compile shader:\n%ws", path );

		SAFE_RELEASE( errorBlob );
		return E_FAIL;
	}

	return S_OK;
}


float ToRad( float deg )
{
	return deg * XM_PI / 180.0f;
}


XMVECTOR ColorToVector( DWORD color )
{
	float r = ( float )( color & 255 ) / 255.0f;
	float g = ( float )( ( color >> 8 ) & 255 ) / 255.0f;
	float b = ( float )( ( color >> 16 ) & 255 ) / 255.0f;
	return XMColorSRGBToRGB( XMVectorSet( r, g, b, 0.0f ) );
}


DWORD VectorToColor( const XMVECTOR& v )
{
	XMVECTOR srgb = XMColorRGBToSRGB( v );
	DWORD ret = 0;
	ret |= ( DWORD )( srgb.m128_f32[ 0 ] * 255.0f );
	ret |= ( DWORD )( srgb.m128_f32[ 1 ] * 255.0f ) << 8;
	ret |= ( DWORD )( srgb.m128_f32[ 2 ] * 255.0f ) << 16;
	return ret;
}