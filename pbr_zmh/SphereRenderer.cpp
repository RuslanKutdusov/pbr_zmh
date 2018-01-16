#include "Precompiled.h"

using namespace DirectX;


struct InstanceParams
{
	XMMATRIX WorldMatrix;
	float Metalness;
	float Roughness;
	float padding[ 2 ];
};


SphereRenderer::SphereRenderer()
{

}


HRESULT SphereRenderer::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	V_RETURN( ReloadShaders( pd3dDevice ) );

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof( InstanceParams );
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &m_instanceBuf ) );
	DXUT_SetDebugName( m_instanceBuf, "InstanceParams" );

	V_RETURN( m_sphereMesh.Create( pd3dDevice, L"Misc\\sphere.sdkmesh" ) );

	return S_OK;
}


void SphereRenderer::Render( const DirectX::XMMATRIX& world, float metalness, float roughness, ID3D11DeviceContext* pd3dImmediateContext )
{
	pd3dImmediateContext->IASetInputLayout( m_inputLayout );

	ID3D11Buffer* pVB = m_sphereMesh.GetVB11( 0, 0 );
	UINT Strides = ( UINT )m_sphereMesh.GetVertexStride( 0, 0 );
	UINT Offsets = 0;
	pd3dImmediateContext->IASetVertexBuffers( 0, 1, &pVB, &Strides, &Offsets );
	pd3dImmediateContext->IASetIndexBuffer( m_sphereMesh.GetIB11( 0 ), m_sphereMesh.GetIBFormat11( 0 ), 0 );

	{
		D3D11_MAPPED_SUBRESOURCE mappedSubres;
		pd3dImmediateContext->Map( m_instanceBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );

		InstanceParams* instanceParams = ( InstanceParams* )mappedSubres.pData;
		instanceParams->WorldMatrix = world;
		instanceParams->Metalness = metalness;
		instanceParams->Roughness = roughness;

		pd3dImmediateContext->Unmap( m_instanceBuf, 0 );

		// apply
		pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &m_instanceBuf );
		pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_instanceBuf );
	}

	pd3dImmediateContext->VSSetShader( m_vs, nullptr, 0 );
	pd3dImmediateContext->PSSetShader( m_ps, nullptr, 0 );

	for( UINT subset = 0; subset < m_sphereMesh.GetNumSubsets( 0 ); ++subset )
	{
		// Get the subset
		auto pSubset = m_sphereMesh.GetSubset( 0, subset );

		auto PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
		pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

		pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	}
}


void SphereRenderer::OnD3D11DestroyDevice()
{
	SAFE_RELEASE( m_inputLayout );
	SAFE_RELEASE( m_vs );
	SAFE_RELEASE( m_ps );
	SAFE_RELEASE( m_instanceBuf );
	m_sphereMesh.Destroy();
}


HRESULT	SphereRenderer::ReloadShaders( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	ID3D11VertexShader* newVs = nullptr;
	ID3D11PixelShader* newPs = nullptr;
	ID3D11InputLayout* newInputLayout = nullptr;

	ID3DBlob* vsBlob = nullptr;
	V_RETURN( CompileShader( L"shaders\\sphere.hlsl", nullptr, "vs_main", SHADER_VERTEX, &vsBlob ) );
	V_RETURN( pd3dDevice->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &newVs ) );
	DXUT_SetDebugName( newVs, "SphereVS" );

	ID3DBlob* psBlob = nullptr;
	V_RETURN( CompileShader( L"shaders\\sphere.hlsl", nullptr, "ps_main", SHADER_PIXEL, &psBlob ) );
	V_RETURN( pd3dDevice->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &newPs ) );
	DXUT_SetDebugName( newPs, "SpherePS" );
	SAFE_RELEASE( psBlob );

	const D3D11_INPUT_ELEMENT_DESC elementsDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	int iNumElements = sizeof( elementsDesc ) / sizeof( D3D11_INPUT_ELEMENT_DESC );

	V_RETURN( pd3dDevice->CreateInputLayout( elementsDesc, iNumElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &newInputLayout ) );
	SAFE_RELEASE( vsBlob );

	//
	SAFE_RELEASE( m_vs );
	SAFE_RELEASE( m_ps );
	SAFE_RELEASE( m_inputLayout );
	m_vs = newVs;
	m_ps = newPs;
	m_inputLayout = newInputLayout;

	return S_OK;
}