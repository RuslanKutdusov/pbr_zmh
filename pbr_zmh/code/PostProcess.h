#pragma once


class PostProcess
{
public:
	PostProcess();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void    Render( ID3D11DeviceContext* pd3dImmediateContext, ID3D11ShaderResourceView* directLightSRV, ID3D11ShaderResourceView* indirectLightSRV, float exposure );
	void    OnD3D11DestroyDevice();
	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

private:

	ID3D11VertexShader* m_quadVs = nullptr;
	ID3D11PixelShader* m_tonemapPs = nullptr;
	ID3D11DepthStencilState* m_depthStencilState = nullptr;
	ID3D11SamplerState* m_samperState = nullptr;
	ID3D11Buffer* m_paramsBuf = nullptr;
};
