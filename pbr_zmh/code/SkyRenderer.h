#pragma once


class SkyRenderer
{
public:
	SkyRenderer();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void	BakeCubemap( ID3D11DeviceContext* pd3dImmediateContext );
	void    RenderDepthPass( ID3D11DeviceContext* pd3dImmediateContext );
	void    RenderLightPass( ID3D11DeviceContext* pd3dImmediateContext );
	void    OnD3D11DestroyDevice();
	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

	HRESULT LoadSkyTexture( ID3D11Device* pd3dDevice, const char* filename );
	ID3D11ShaderResourceView* GetCubeMapSRV() const { return m_cubeMapSRV; }

public:

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;
	ID3D11GeometryShader* m_gs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;
	ID3D11ShaderResourceView* m_skyTextureSRV = nullptr;
	bool m_isSkyTextureCube;
	ID3D11Buffer* m_instanceBuf = nullptr;
	Model m_sphereModel;
	ID3D11Texture2D* m_cubeMapTexture;
	ID3D11RenderTargetView* m_cubeMapRTV;
	ID3D11ShaderResourceView* m_cubeMapSRV;

	ID3D11Texture2D* m_dummyTexture;
	ID3D11ShaderResourceView* m_dummyCubeSRV;
	ID3D11ShaderResourceView* m_dummy2DSRV;

	struct InstanceParams
	{
		DirectX::XMMATRIX BakeViewProj[ 6 ];
		UINT Bake;
		UINT UseCubeTexture;
		UINT padding[ 2 ];
	};
	void Draw( ID3D11DeviceContext* pd3dImmediateContext, UINT numInstances, InstanceParams& params );
	void ApplyResources( ID3D11DeviceContext* pd3dImmediateContext );
};
