#pragma once


struct MERLMaterial
{
	HRESULT Load( ID3D11Device* pd3dDevice, const char* materialName );
	void Release();

	ID3D11Buffer* m_brdf = nullptr;
	ID3D11ShaderResourceView* m_brdfSRV = nullptr;
};
