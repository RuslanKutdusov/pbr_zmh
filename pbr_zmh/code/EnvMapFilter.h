#pragma once


class EnvMapFilter
{
public:
	EnvMapFilter();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void	FilterEnvMap( ID3D11DeviceContext* pd3dImmediateContext, UINT envMapSlot, ID3D11ShaderResourceView* envmap );
	void    OnD3D11DestroyDevice();

	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

private:
	ID3D11ComputeShader* m_envMapPrefilter = nullptr;
	ID3D11ComputeShader* m_brdfLutGen = nullptr;
	ID3D11Buffer* m_m_envMapPrefilterCb = nullptr;

	ID3D11Texture2D* m_prefilteredEnvMap = nullptr;
	ID3D11UnorderedAccessView** m_prefilteredEnvMapUAV = nullptr;
	ID3D11Texture2D* m_brdfLut = nullptr;
	ID3D11UnorderedAccessView* m_brdfLutUav = nullptr;
};
