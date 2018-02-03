#pragma once


class SphereRenderer
{
public:
	struct InstanceParams
	{
		DirectX::XMMATRIX WorldMatrix;
		float Metalness;
		float Roughness;
		float Reflectance;
		float padding0[ 1 ];
		DirectX::XMVECTOR Albedo;
		UINT UseMaterial;
		UINT padding1[ 3 ];
	};
	const UINT MAX_INSTANCES = 128;

	SphereRenderer();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void	RenderDepthPass( InstanceParams* instancesParams, UINT numInstances, ID3D11DeviceContext* pd3dImmediateContext );
	void    RenderLightPass( InstanceParams* instancesParams, Material* material, UINT numInstances, ID3D11DeviceContext* pd3dImmediateContext );
	void	RenderPlaneDepthPass( DirectX::XMMATRIX metalnessPlane, DirectX::XMMATRIX roughnessPlane, ID3D11DeviceContext* pd3dImmediateContext );
	void    RenderPlaneLightPass( DirectX::XMMATRIX metalnessPlane, DirectX::XMMATRIX roughnessPlane, ID3D11DeviceContext* pd3dImmediateContext );
	void    OnD3D11DestroyDevice();
	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

private:

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;
	ID3D11Buffer* m_instanceBuf = nullptr;
	Model m_sphereModel;

	ID3D11VertexShader* m_planeVs = nullptr;
	ID3D11PixelShader* m_planePs = nullptr;
	ID3D11Buffer* m_planeInstanceBuf = nullptr;
	ID3D11ShaderResourceView* m_metalnessTexture;
	ID3D11ShaderResourceView* m_roughnessTexture;
	Model m_planeModel;

	void    Render( InstanceParams* instancesParams, UINT numInstances, ID3D11PixelShader* ps, ID3D11DeviceContext* pd3dImmediateContext );
	void    RenderPlane( DirectX::XMMATRIX metalnessPlane, DirectX::XMMATRIX roughnessPlane, ID3D11PixelShader* ps, ID3D11DeviceContext* pd3dImmediateContext );
};
