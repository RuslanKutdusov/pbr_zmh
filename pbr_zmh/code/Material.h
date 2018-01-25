#pragma once
#include <string>


struct Material
{
	HRESULT Load( ID3D11Device* pd3dDevice, const wchar_t* materialName );
	HRESULT Load( ID3D11Device* pd3dDevice, const wchar_t* materialName, const wchar_t* albedoPath, const wchar_t* normalPath,
		const wchar_t* roughnessPath, const wchar_t* metalnessPath );
	void Release();

	ID3D11ShaderResourceView* albedo = nullptr;
	ID3D11ShaderResourceView* normal = nullptr;
	ID3D11ShaderResourceView* roughness = nullptr;
	ID3D11ShaderResourceView* metalness = nullptr;
	std::wstring name;
};
