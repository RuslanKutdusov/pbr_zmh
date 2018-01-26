#include "Precompiled.h"

using namespace DirectX;


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
	Desc.ByteWidth = sizeof( InstanceParams ) * MAX_INSTANCES;
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &m_instanceBuf ) );
	DXUT_SetDebugName( m_instanceBuf, "InstanceParams" );

	V_RETURN( m_sphereModel.Load( pd3dDevice, L"Misc", L"sphere.obj", false, TextureMapping() ) );

	return S_OK;
}


void SphereRenderer::Render( InstanceParams* instancesParams, UINT numInstances, ID3D11PixelShader* ps, ID3D11DeviceContext* pd3dImmediateContext )
{
	pd3dImmediateContext->IASetInputLayout( m_inputLayout );

	{
		D3D11_MAPPED_SUBRESOURCE mappedSubres;
		pd3dImmediateContext->Map( m_instanceBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );
		memcpy( mappedSubres.pData, instancesParams, sizeof( InstanceParams ) * numInstances );
		pd3dImmediateContext->Unmap( m_instanceBuf, 0 );

		// apply
		pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &m_instanceBuf );
		pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_instanceBuf );
	}

	pd3dImmediateContext->VSSetShader( m_vs, nullptr, 0 );
	pd3dImmediateContext->PSSetShader( ps, nullptr, 0 );

	for( uint32_t i = 0; i < m_sphereModel.meshesNum; i++ )
	{
		Mesh& mesh = m_sphereModel.meshes[ i ];
		mesh.Apply( pd3dImmediateContext );
		pd3dImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		pd3dImmediateContext->DrawIndexedInstanced( mesh.indexCount, numInstances, 0, 0, 0 );
	}
}


void SphereRenderer::RenderDepthPass(InstanceParams* instancesParams, UINT numInstances, ID3D11DeviceContext* pd3dImmediateContext)
{
	Render( instancesParams, numInstances, nullptr, pd3dImmediateContext );
}


void SphereRenderer::RenderLightPass( InstanceParams* instancesParams, Material* material, UINT numInstances, ID3D11DeviceContext* pd3dImmediateContext )
{
	ID3D11ShaderResourceView* srv[ 4 ];
	memset( srv, 0, sizeof( ID3D11ShaderResourceView* ) * 4 );
	if( material )
	{
		srv[ 0 ] = material->albedo;
		srv[ 1 ] = material->normal;
		srv[ 2 ] = material->roughness;
		srv[ 3 ] = material->metalness;
	}
	pd3dImmediateContext->PSSetShaderResources( 0, 4, srv );
	Render( instancesParams, numInstances, m_ps, pd3dImmediateContext );
}


void SphereRenderer::OnD3D11DestroyDevice()
{
	SAFE_RELEASE( m_inputLayout );
	SAFE_RELEASE( m_vs );
	SAFE_RELEASE( m_ps );
	SAFE_RELEASE( m_instanceBuf );
	m_sphereModel.Release();
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
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE",	0, DXGI_FORMAT_R32G32_FLOAT,	2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL",	0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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