#include "Precompiled.h"

using namespace DirectX;


const UINT CUBEMAP_RESOLUTION = 256;
const UINT CUBEMAP_MIPS = 9;


SkyRenderer::SkyRenderer()
{

}


HRESULT SkyRenderer::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	
	V_RETURN( ReloadShaders( pd3dDevice ) );
	V_RETURN( m_sphereModel.Load( pd3dDevice, L"Misc", L"sphere.obj", false, TextureMapping() ) );

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
		texDesc.MipLevels = CUBEMAP_MIPS;
		texDesc.SampleDesc.Count = 1;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
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
		srvDesc.TextureCube.MipLevels = CUBEMAP_MIPS;
		srvDesc.TextureCube.MostDetailedMip = 0;
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_cubeMapTexture, &srvDesc, &m_cubeMapSRV ) );
	}

	return S_OK;
}


void SkyRenderer::ApplyResources( ID3D11DeviceContext* pd3dImmediateContext )
{
	pd3dImmediateContext->IASetInputLayout( m_inputLayout );

	pd3dImmediateContext->VSSetShader( m_vs, nullptr, 0 );
	pd3dImmediateContext->GSSetShader( m_gs, nullptr, 0 );
	pd3dImmediateContext->PSSetShaderResources( 0, 1, &m_skyTextureSRV );
	pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &m_instanceBuf );
	pd3dImmediateContext->GSSetConstantBuffers( 1, 1, &m_instanceBuf );
	pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_instanceBuf );
}


void SkyRenderer::BakeCubemap( ID3D11DeviceContext* pd3dImmediateContext )
{
	pd3dImmediateContext->PSSetShader( m_ps, nullptr, 0 );
	ApplyResources( pd3dImmediateContext );

	// save state
	ID3D11RenderTargetView* savedRTVs[ D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT ];
	ID3D11DepthStencilView* savedDSV;
	D3D11_VIEWPORT savedViewport;
	UINT numViewports = 1;
	pd3dImmediateContext->RSGetViewports( &numViewports, &savedViewport );
	pd3dImmediateContext->OMGetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, savedRTVs, &savedDSV );

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
	pd3dImmediateContext->OMSetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, savedRTVs, savedDSV );
	pd3dImmediateContext->RSSetViewports( numViewports, &savedViewport );
	for( UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++ )
		SAFE_RELEASE( savedRTVs[ i ] );
	SAFE_RELEASE( savedDSV );
	pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
	ID3D11Buffer* nullBuffer = nullptr;
	pd3dImmediateContext->GSSetConstantBuffers( 1, 1, &nullBuffer );

	pd3dImmediateContext->GenerateMips( m_cubeMapSRV );
}


void SkyRenderer::RenderDepthPass( ID3D11DeviceContext* pd3dImmediateContext )
{
	pd3dImmediateContext->PSSetShader( nullptr, nullptr, 0 );
	ApplyResources( pd3dImmediateContext );

	InstanceParams params;
	params.Bake = 0;
	Draw( pd3dImmediateContext, 1, params );

	// disable geometry shader
	pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
	ID3D11Buffer* nullBuffer = nullptr;
	pd3dImmediateContext->GSSetConstantBuffers( 1, 1, &nullBuffer );
}


void SkyRenderer::RenderLightPass( ID3D11DeviceContext* pd3dImmediateContext )
{
	pd3dImmediateContext->PSSetShader( m_ps, nullptr, 0 );
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

	for( uint32_t i = 0; i < m_sphereModel.meshesNum; i++ )
	{
		Mesh& mesh = m_sphereModel.meshes[ i ];
		mesh.Apply( pd3dImmediateContext );
		pd3dImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		pd3dImmediateContext->DrawIndexedInstanced( mesh.indexCount, numInstances, 0, 0, 0 );
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
	m_sphereModel.Release();
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