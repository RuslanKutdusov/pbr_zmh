#pragma once


enum SHADER_TYPE
{
	SHADER_VERTEX = 0,
	SHADER_GEOMETRY,
	SHADER_PIXEL
};


//
void ErrorMessageBox( const char* text, ... );
HRESULT CompileShader( LPCWSTR path, const D3D_SHADER_MACRO* pDefines, LPCSTR pEntrypoint, SHADER_TYPE shaderType, ID3DBlob** bytecodeBlob );
float ToRad( float deg );
DirectX::XMVECTOR ColorToVector( DWORD color );
DWORD VectorToColor( const DirectX::XMVECTOR& v );