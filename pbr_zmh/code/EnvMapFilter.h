#pragma once


class EnvMapFilter
{
public:
	EnvMapFilter();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void	FilterEnvMap( ID3D11DeviceContext* pd3dImmediateContext, UINT envMapSlot, ID3D11ShaderResourceView* envmap );
	void    OnD3D11DestroyDevice();

	ID3D11ShaderResourceView* GetPrefilteredSpecEnvMap();
	ID3D11ShaderResourceView* GetBRDFLut();
	ID3D11ShaderResourceView* GetPrefilteredDiffEnvMap();

	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

private:
	ID3D11ComputeShader* m_envMapSpecPrefilter = nullptr;
	ID3D11ComputeShader* m_brdfLutGen = nullptr;
	ID3D11ComputeShader* m_envMapDiffPrefilter = nullptr;

	ID3D11Buffer* m_envMapPrefilterCb = nullptr;

	ID3D11Texture2D* m_prefilteredSpecEnvMap = nullptr;
	ID3D11UnorderedAccessView** m_prefilteredSpecEnvMapUAV = nullptr;
	ID3D11ShaderResourceView* m_prefilteredSpecEnvMapSRV = nullptr;

	ID3D11Texture2D* m_brdfLut = nullptr;
	ID3D11UnorderedAccessView* m_brdfLutUav = nullptr;
	ID3D11ShaderResourceView* m_brdfLutSRV = nullptr;

	ID3D11Texture2D* m_prefilteredDiffEnvMap = nullptr;
	ID3D11UnorderedAccessView* m_prefilteredDiffEnvMapUAV = nullptr;
	ID3D11ShaderResourceView* m_prefilteredDiffEnvMapSRV = nullptr;
};
