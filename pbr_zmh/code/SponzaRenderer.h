#pragma once

class SponzaRenderer
{
public:
	SponzaRenderer();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void	RenderDepthPass( ID3D11DeviceContext* pd3dImmediateContext );
	void    RenderLightPass( ID3D11DeviceContext* pd3dImmediateContext, const DirectX::XMVECTOR& pointLightFlux, float fTime );
	void    OnD3D11DestroyDevice();
	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

private:
	Model m_model;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;
	ID3D11Buffer* m_instanceBuf = nullptr;

	void Render( ID3D11DeviceContext* pd3dImmediateContext, bool depthPass );
};
