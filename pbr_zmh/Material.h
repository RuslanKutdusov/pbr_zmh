#pragma once
#include <string>


struct Material
{
	HRESULT Load( ID3D11Device* pd3dDevice, const wchar_t* name );
	void Release();

	ID3D11ShaderResourceView* albedo = nullptr;
	ID3D11ShaderResourceView* normal = nullptr;
	ID3D11ShaderResourceView* roughness = nullptr;
	ID3D11ShaderResourceView* metalness = nullptr;
	std::wstring name;
};
