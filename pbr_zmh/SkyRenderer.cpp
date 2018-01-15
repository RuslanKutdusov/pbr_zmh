#include "Precompiled.h"

using namespace DirectX;


SkyRenderer::SkyRenderer()
{

}


HRESULT SkyRenderer::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	WCHAR str[ MAX_PATH ];
	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"shaders\\sky.hlsl" ) );

	ID3DBlob* vsBlob = nullptr;
	V_RETURN( DXUTCompileFromFile( str, nullptr, "vs_main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG, 0, &vsBlob ) );
	V_RETURN( pd3dDevice->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs ) );
	DXUT_SetDebugName( m_vs, "SkyVS" );

	ID3DBlob* psBlob = nullptr;
	V_RETURN( DXUTCompileFromFile( str, nullptr, "ps_main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG, 0, &psBlob ) );
	V_RETURN( pd3dDevice->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps ) );
	DXUT_SetDebugName( m_ps, "SkyPS" );
	SAFE_RELEASE( psBlob );

	const D3D11_INPUT_ELEMENT_DESC elementsDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	int iNumElements = sizeof( elementsDesc ) / sizeof( D3D11_INPUT_ELEMENT_DESC );

	V_RETURN( pd3dDevice->CreateInputLayout( elementsDesc, iNumElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout ) );
	SAFE_RELEASE( vsBlob );

	V_RETURN( m_sphereMesh.Create( pd3dDevice, L"Misc\\skysphere.sdkmesh" ) );

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[ 0 ] = samplerDesc.BorderColor[ 1 ] = samplerDesc.BorderColor[ 2 ] = samplerDesc.BorderColor[ 3 ] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN( pd3dDevice->CreateSamplerState( &samplerDesc, &m_samperState ) );

	V_RETURN( DXUTCreateShaderResourceViewFromFile( pd3dDevice, L"HDRs\\noon_grass_2k.dds", &m_textureSRV ) );

	return S_OK;
}


void SkyRenderer::Render( ID3D11DeviceContext* pd3dImmediateContext )
{
	pd3dImmediateContext->IASetInputLayout( m_inputLayout );

	ID3D11Buffer* pVB = m_sphereMesh.GetVB11( 0, 0 );
	UINT Strides = ( UINT )m_sphereMesh.GetVertexStride( 0, 0 );
	UINT Offsets = 0;
	pd3dImmediateContext->IASetVertexBuffers( 0, 1, &pVB, &Strides, &Offsets );
	pd3dImmediateContext->IASetIndexBuffer( m_sphereMesh.GetIB11( 0 ), m_sphereMesh.GetIBFormat11( 0 ), 0 );

	pd3dImmediateContext->VSSetShader( m_vs, nullptr, 0 );
	pd3dImmediateContext->PSSetShader( m_ps, nullptr, 0 );
	pd3dImmediateContext->PSSetShaderResources( 0, 1, &m_textureSRV );
	pd3dImmediateContext->PSSetSamplers( 0, 1, &m_samperState );

	for( UINT subset = 0; subset < m_sphereMesh.GetNumSubsets( 0 ); ++subset )
	{
		// Get the subset
		auto pSubset = m_sphereMesh.GetSubset( 0, subset );

		auto PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
		pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

		pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	}
}


void SkyRenderer::OnD3D11DestroyDevice()
{
	SAFE_RELEASE( m_inputLayout );
	SAFE_RELEASE( m_vs );
	SAFE_RELEASE( m_ps );
	SAFE_RELEASE( m_textureSRV );
	SAFE_RELEASE( m_samperState );
	m_sphereMesh.Destroy();
}