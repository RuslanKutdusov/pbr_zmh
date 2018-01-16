#include "Precompiled.h"

using namespace DirectX;


const UINT CUBEMAP_RESOLUTION = 256;


SkyRenderer::SkyRenderer()
{

}


HRESULT SkyRenderer::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	
	V_RETURN( ReloadShaders( pd3dDevice ) );
	V_RETURN( m_sphereMesh.Create( pd3dDevice, L"Misc\\sphere.sdkmesh" ) );

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof( InstanceParams );
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &m_instanceBuf ) );
	DXUT_SetDebugName( m_instanceBuf, "InstanceParams" );

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


void SkyRenderer::ApplyResources( ID3D11DeviceContext* pd3dImmediateContext )
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
	pd3dImmediateContext->PSSetShaderResources( 0, 1, &m_skyTextureSRV );
	pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &m_instanceBuf );
	pd3dImmediateContext->GSSetConstantBuffers( 1, 1, &m_instanceBuf );
	pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_instanceBuf );
}


void SkyRenderer::BakeCubemap( ID3D11DeviceContext* pd3dImmediateContext )
{
	ApplyResources( pd3dImmediateContext );

	// save state
	ID3D11RenderTargetView* savedRTV;
	ID3D11DepthStencilView* savedDSV;
	D3D11_VIEWPORT savedViewport;
	UINT numViewports = 1;
	pd3dImmediateContext->RSGetViewports( &numViewports, &savedViewport );
	pd3dImmediateContext->OMGetRenderTargets( 1, &savedRTV, &savedDSV );

	InstanceParams params;
	params.Bake = ~0u;
	for( int i = 0; i < 6; i++ )
	{
		XMMATRIX view = DXUTGetCubeMapViewMatrix( i );
		XMMATRIX proj = XMMatrixPerspectiveFovLH( XM_PI / 2.0f, 1.0f, 1.0f, 5000.0f );
		params.BakeViewProj[ i ] = XMMatrixMultiply( view, proj );
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
	pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
	ID3D11Buffer* nullBuffer = nullptr;
	pd3dImmediateContext->GSSetConstantBuffers( 1, 1, &nullBuffer );
}


void SkyRenderer::Render( ID3D11DeviceContext* pd3dImmediateContext )
{
	ApplyResources( pd3dImmediateContext );

	InstanceParams params;
	params.Bake = 0;
	Draw( pd3dImmediateContext, 1, params );

	// disable geometry shader
	pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
	ID3D11Buffer* nullBuffer = nullptr;
	pd3dImmediateContext->GSSetConstantBuffers( 1, 1, &nullBuffer );
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
	SAFE_RELEASE( m_skyTextureSRV );
	SAFE_RELEASE( m_instanceBuf );
	SAFE_RELEASE( m_cubeMapTexture );
	SAFE_RELEASE( m_cubeMapRTV );
	SAFE_RELEASE( m_cubeMapSRV );
	m_sphereMesh.Destroy();
}


HRESULT	SkyRenderer::ReloadShaders( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	ID3D11VertexShader* newVs = nullptr;
	ID3D11GeometryShader* newGs = nullptr;
	ID3D11PixelShader* newPs = nullptr;
	ID3D11InputLayout* newInputLayout = nullptr;

	ID3DBlob* vsBlob = nullptr;
	V_RETURN( CompileShader( L"shaders\\sky.hlsl", nullptr, "vs_main", SHADER_VERTEX, &vsBlob ) );
	V_RETURN( pd3dDevice->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &newVs ) );
	DXUT_SetDebugName( newVs, "SkyVS" );

	ID3DBlob* gsBlob = nullptr;
	V_RETURN( CompileShader( L"shaders\\sky.hlsl", nullptr, "gs_main", SHADER_GEOMETRY, &gsBlob ) );
	V_RETURN( pd3dDevice->CreateGeometryShader( gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), nullptr, &newGs ) );
	DXUT_SetDebugName( newGs, "SkyGS" );
	SAFE_RELEASE( gsBlob );

	ID3DBlob* psBlob = nullptr;
	V_RETURN( CompileShader( L"shaders\\sky.hlsl", nullptr, "ps_main", SHADER_PIXEL, &psBlob ) );
	V_RETURN( pd3dDevice->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &newPs ) );
	DXUT_SetDebugName( newPs, "SkyPS" );
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
	SAFE_RELEASE( m_gs );
	SAFE_RELEASE( m_ps );
	SAFE_RELEASE( m_inputLayout );
	m_vs = newVs;
	m_gs = newGs;
	m_ps = newPs;
	m_inputLayout = newInputLayout;

	return S_OK;
}


HRESULT SkyRenderer::LoadSkyTexture( ID3D11Device* pd3dDevice, const wchar_t* filename )
{
	HRESULT hr;
	SAFE_RELEASE( m_skyTextureSRV );
	V_RETURN( DXUTCreateShaderResourceViewFromFile( pd3dDevice, filename, &m_skyTextureSRV ) );

	return S_OK;
}