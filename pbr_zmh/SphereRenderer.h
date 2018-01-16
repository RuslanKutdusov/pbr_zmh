#pragma once


class SphereRenderer
{
public:
	SphereRenderer();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void    Render( const DirectX::XMMATRIX& world, float metalness, float roughness, ID3D11DeviceContext* pd3dImmediateContext );
	void    OnD3D11DestroyDevice();
	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

private:

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;
	ID3D11Buffer* m_instanceBuf = nullptr;
	CDXUTSDKMesh m_sphereMesh;
};
