#pragma once


class SkyRenderer
{
public:
	SkyRenderer();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void    Render( ID3D11DeviceContext* pd3dImmediateContext );
	void    OnD3D11DestroyDevice();

private:

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;
	ID3D11ShaderResourceView* m_textureSRV = nullptr;
	ID3D11SamplerState* m_samperState = nullptr;
	CDXUTSDKMesh m_sphereMesh;
};
