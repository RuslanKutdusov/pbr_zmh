#pragma once

class SponzaRenderer
{
public:
	SponzaRenderer();

	HRESULT OnD3D11CreateDevice( ID3D11Device* pd3dDevice );
	void	RenderDepthPass( ID3D11DeviceContext* pd3dImmediateContext );
	void    RenderLightPass( ID3D11DeviceContext* pd3dImmediateContext );
	void    OnD3D11DestroyDevice();
	HRESULT	ReloadShaders( ID3D11Device* pd3dDevice );

private:
	Material* m_materials;
	uint32_t m_materialsNum;

	struct Mesh
	{
		ID3D11Buffer* posVb = nullptr;
		ID3D11Buffer* normalVb = nullptr;
		ID3D11Buffer* texVb = nullptr;
		ID3D11Buffer* tangentVb = nullptr;
		ID3D11Buffer* binormalVb = nullptr;
		ID3D11Buffer* ib = nullptr;
		uint32_t materialIdx;
		uint32_t indexCount;
	};
	Mesh* m_meshes;
	uint32_t m_meshesNum;

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;
	ID3D11PixelShader* m_ps = nullptr;

	void Render( ID3D11DeviceContext* pd3dImmediateContext, bool depthPass );
};
