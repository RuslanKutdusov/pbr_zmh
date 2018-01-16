#pragma once


class SkyRenderer
{
public:
	SkyRenderer();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void    Render( ID3D11DeviceContext* pd3dImmediateContext );
	void    OnD3D11DestroyDevice();
	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

	ID3D11ShaderResourceView* GetCubeMapSRV() const { return m_cubeMapSRV; }

public:

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;
	ID3D11GeometryShader* m_gs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;
	ID3D11ShaderResourceView* m_textureSRV = nullptr;
	ID3D11Buffer* m_instanceBuf = nullptr;
	CDXUTSDKMesh m_sphereMesh;
	ID3D11Texture2D* m_cubeMapTexture;
	ID3D11RenderTargetView* m_cubeMapRTV;
	ID3D11ShaderResourceView* m_cubeMapSRV;

	struct InstanceParams
	{
		DirectX::XMMATRIX BakeViewProj[ 6 ];
		UINT Bake;
		UINT padding[ 3 ];
	};
	void Draw( ID3D11DeviceContext* pd3dImmediateContext, UINT numInstances, InstanceParams& params );
};
