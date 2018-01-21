#pragma once


class SphereRenderer
{
public:
	struct InstanceParams
	{
		DirectX::XMMATRIX WorldMatrix;
		float Metalness;
		float Roughness;
		UINT EnableDirectLight;
		UINT EnableIndirectLight;
	};
	const UINT MAX_INSTANCES = 128;

	SphereRenderer();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void	RenderDepthPass( InstanceParams* instancesParams, UINT numInstances, ID3D11DeviceContext* pd3dImmediateContext );
	void    RenderLightPass( InstanceParams* instancesParams, UINT numInstances, ID3D11DeviceContext* pd3dImmediateContext );
	void    OnD3D11DestroyDevice();
	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

private:

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;
	ID3D11Buffer* m_instanceBuf = nullptr;
	CDXUTSDKMesh m_sphereMesh;

	void    Render( InstanceParams* instancesParams, UINT numInstances, ID3D11PixelShader* ps, ID3D11DeviceContext* pd3dImmediateContext );
};
