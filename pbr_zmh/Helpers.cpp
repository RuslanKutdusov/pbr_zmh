#include "Precompiled.h"


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