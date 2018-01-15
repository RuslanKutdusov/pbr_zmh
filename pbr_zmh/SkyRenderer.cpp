#include "Precompiled.h"

using namespace DirectX;


const UINT CUBEMAP_RESOLUTION = 256;


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

	ID3DBlob* gsBlob = nullptr;
	V_RETURN( DXUTCompileFromFile( str, nullptr, "gs_main", "gs_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG, 0, &gsBlob ) );
	V_RETURN( pd3dDevice->CreateGeometryShader( gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), nullptr, &m_gs ) );
	DXUT_SetDebugName( m_gs, "SkyGS" );
	SAFE_RELEASE( gsBlob );

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

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof( InstanceParams );
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &m_instanceBuf ) );
	DXUT_SetDebugName( m_instanceBuf, "InstanceParams" );

	V_RETURN( DXUTCreateShaderResourceViewFromFile( pd3dDevice, L"HDRs\\noon_grass_2k.dds", &m_textureSRV ) );

	// cube map
	{
		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory( &texDesc, sizeof( D3D11_TEXTURE2D_DESC ) );
		texDesc.ArraySize = 6;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.Width = CUBEMAP_RESOLUTION;
		texDesc.Height = CUBEMAP_RESOLUTION;
		texDesc.MipLevels = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, nullptr, &m_cubeMapTexture ) );
		DXUT_SetDebugName( m_cubeMapTexture, "CubeMapTexture" );

		// Create the render target view
		D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
		rtDesc.Format = texDesc.Format;
		rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtDesc.Texture2DArray.ArraySize = 6;
		rtDesc.Texture2DArray.FirstArraySlice = 0;
		rtDesc.Texture2DArray.MipSlice = 0;
		V_RETURN( pd3dDevice->CreateRenderTargetView( m_cubeMapTexture, &rtDesc, &m_cubeMapRTV ) );
		DXUT_SetDebugName( m_cubeMapRTV, "CubeMapRTV" );

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_cubeMapTexture, &srvDesc, &m_cubeMapSRV ) );
	}

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
	pd3dImmediateContext->GSSetShader( m_gs, nullptr, 0 );
	pd3dImmediateContext->PSSetShader( m_ps, nullptr, 0 );
	pd3dImmediateContext->PSSetShaderResources( 0, 1, &m_textureSRV );
	pd3dImmediateContext->PSSetSamplers( 0, 1, &m_samperState );
	pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &m_instanceBuf );
	pd3dImmediateContext->GSSetConstantBuffers( 1, 1, &m_instanceBuf );
	pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_instanceBuf );

	// bake cubemap
	{
		ID3D11RenderTargetView* savedRTV;
		ID3D11DepthStencilView* savedDSV;
		D3D11_VIEWPORT savedViewport;
		UINT numViewports = 1;
		pd3dImmediateContext->RSGetViewports( &numViewports, &savedViewport );
		pd3dImmediateContext->OMGetRenderTargets( 1, &savedRTV, &savedDSV );

		InstanceParams params;
		for( int i = 0; i < 6; i++ )
		{
			XMVECTOR camAt = XMVectorZero(), camUp = XMVectorZero();
			switch( i )
			{
				case 0:
					camAt = XMVectorSet( 1.0f, 0.0f, 0.0f, 0.0f );
					camUp = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
					break;
				case 1:
					camAt = XMVectorSet( -1.0f, 0.0f, 0.0f, 0.0f );
					camUp = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
					break;
				case 2:
					camAt = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
					camUp = XMVectorSet( 0.0f, 0.0f, -1.0f, 0.0f );
					break;
				case 3:
					camAt = XMVectorSet( 0.0f, -1.0f, 0.0f, 0.0f );
					camUp = XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f );
					break;
				case 4:
					camAt = XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f );
					camUp = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
					break;
				case 5:
					camAt = XMVectorSet( 0.0f, 0.0f, -1.0f, 0.0f );
					camUp = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
					break;
			}

			XMMATRIX view = XMMatrixLookAtLH( XMVectorZero(), camAt, camUp );
			XMMATRIX proj = XMMatrixPerspectiveFovLH( XM_PI / 2.0f, 1.0f, 1.0f, 5000.0f );
			params.BakeViewProj[ i ] = XMMatrixMultiply( view, proj );
			params.Bake = ~0u;
		}

		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = CUBEMAP_RESOLUTION;
		viewport.Height = CUBEMAP_RESOLUTION;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		pd3dImmediateContext->OMSetRenderTargets( 1, &m_cubeMapRTV, nullptr );
		pd3dImmediateContext->RSSetViewports( 1, &viewport );
		pd3dImmediateContext->ClearRenderTargetView( m_cubeMapRTV, Colors::Black );
		Draw( pd3dImmediateContext, 6, params );

		// restore state
		pd3dImmediateContext->OMSetRenderTargets( 1, &savedRTV, savedDSV );
		pd3dImmediateContext->RSSetViewports( numViewports, &savedViewport );
		SAFE_RELEASE( savedRTV );
		SAFE_RELEASE( savedDSV );
	}

	InstanceParams params;
	params.Bake = 0;
	Draw( pd3dImmediateContext, 1, params );

	pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
}


void SkyRenderer::Draw( ID3D11DeviceContext* pd3dImmediateContext, UINT numInstances, InstanceParams& params )
{
	D3D11_MAPPED_SUBRESOURCE mappedSubres;
	pd3dImmediateContext->Map( m_instanceBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );
	memcpy( mappedSubres.pData, &params, sizeof( InstanceParams ) );
	pd3dImmediateContext->Unmap( m_instanceBuf, 0 );

	for( UINT subset = 0; subset < m_sphereMesh.GetNumSubsets( 0 ); ++subset )
	{
		// Get the subset
		auto pSubset = m_sphereMesh.GetSubset( 0, subset );

		auto PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
		pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

		pd3dImmediateContext->DrawIndexedInstanced( ( UINT )pSubset->IndexCount, numInstances, 0, ( UINT )pSubset->VertexStart, 0 );
	}
}


void SkyRenderer::OnD3D11DestroyDevice()
{
	SAFE_RELEASE( m_inputLayout );
	SAFE_RELEASE( m_vs );
	SAFE_RELEASE( m_gs );
	SAFE_RELEASE( m_ps );
	SAFE_RELEASE( m_textureSRV );
	SAFE_RELEASE( m_samperState );
	SAFE_RELEASE( m_instanceBuf );
	SAFE_RELEASE( m_cubeMapTexture );
	SAFE_RELEASE( m_cubeMapRTV );
	SAFE_RELEASE( m_cubeMapSRV );
	m_sphereMesh.Destroy();
}