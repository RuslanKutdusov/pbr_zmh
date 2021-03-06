#include "Precompiled.h"


const UINT SPEC_CUBEMAP_RESOLUTION = 256;
const UINT SPEC_CUBEMAP_MIPS = 9;
const UINT DIFF_CUBEMAP_RESOLUTION = 128;
const UINT DIFF_CUBEMAP_MIPS = 1;
const UINT BRDF_LUT_SIZE = 256;
const UINT THREAD_GROUP_SIZE = 32;


struct Constants
{
	UINT MipIndex;
	UINT MipsNumber;
	UINT padding[ 2 ];
};


UINT ComputeMipSize( UINT mainSliceSize, UINT mipIndex )
{
	return std::max( 1u, mainSliceSize >> mipIndex );
}


EnvMapFilter::EnvMapFilter()
{

}


HRESULT EnvMapFilter::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
	HRESULT hr;

	V_RETURN( ReloadShaders( pd3dDevice ) );

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof( Constants );
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &m_envMapPrefilterCb ) );
	DXUT_SetDebugName( m_envMapPrefilterCb, "Constants" );

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory( &texDesc, sizeof( D3D11_TEXTURE2D_DESC ) );
	texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;

	// prefiltered specular env map
	texDesc.ArraySize = 6;
	texDesc.Width = SPEC_CUBEMAP_RESOLUTION;
	texDesc.Height = SPEC_CUBEMAP_RESOLUTION;
	texDesc.MipLevels = SPEC_CUBEMAP_MIPS;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, nullptr, &m_prefilteredSpecEnvMap ) );
	DXUT_SetDebugName( m_prefilteredSpecEnvMap, "PrefilteredSpecEnvMap" );

	m_prefilteredSpecEnvMapUAV = new ID3D11UnorderedAccessView*[ SPEC_CUBEMAP_MIPS ];
	for( UINT i = 0; i < SPEC_CUBEMAP_MIPS; i++ )
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory( &uavDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
		uavDesc.Format = texDesc.Format;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = 6;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = i;

		V_RETURN( pd3dDevice->CreateUnorderedAccessView( m_prefilteredSpecEnvMap, &uavDesc, &m_prefilteredSpecEnvMapUAV[ i ] ) );
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory( &srvDesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;
	V_RETURN( pd3dDevice->CreateShaderResourceView( m_prefilteredSpecEnvMap, &srvDesc, &m_prefilteredSpecEnvMapSRV ) );

	// BRDF Lut
	texDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	texDesc.ArraySize = 1;
	texDesc.Width = BRDF_LUT_SIZE;
	texDesc.Height = texDesc.Width;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, nullptr, &m_brdfLut ) );
	DXUT_SetDebugName( m_brdfLut, "BRDF Lut" );

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory( &uavDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
	uavDesc.Format = texDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateUnorderedAccessView( m_brdfLut, &uavDesc, &m_brdfLutUav ) );

	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	V_RETURN( pd3dDevice->CreateShaderResourceView( m_brdfLut, &srvDesc, &m_brdfLutSRV ) );

	// prefiltered diffuse env map 
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.ArraySize = 6;
	texDesc.Width = DIFF_CUBEMAP_RESOLUTION;
	texDesc.Height = DIFF_CUBEMAP_RESOLUTION;
	texDesc.MipLevels = DIFF_CUBEMAP_MIPS;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, nullptr, &m_prefilteredDiffEnvMap ) );
	DXUT_SetDebugName( m_prefilteredDiffEnvMap, "PrefilteredDiffEnvMap" );

	ZeroMemory( &uavDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
	uavDesc.Format = texDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	uavDesc.Texture2DArray.ArraySize = 6;
	uavDesc.Texture2DArray.FirstArraySlice = 0;
	uavDesc.Texture2DArray.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateUnorderedAccessView( m_prefilteredDiffEnvMap, &uavDesc, &m_prefilteredDiffEnvMapUAV ) );

	ZeroMemory( &srvDesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;
	V_RETURN( pd3dDevice->CreateShaderResourceView( m_prefilteredDiffEnvMap, &srvDesc, &m_prefilteredDiffEnvMapSRV ) );

	return S_OK;
}


void EnvMapFilter::FilterEnvMap( ID3D11DeviceContext* pd3dImmediateContext, UINT envMapSlot, ID3D11ShaderResourceView* envmap )
{
	pd3dImmediateContext->CSSetConstantBuffers( 1, 1, &m_envMapPrefilterCb );
	pd3dImmediateContext->CSSetShaderResources( envMapSlot, 1, &envmap );

	for( UINT i = 0; i < SPEC_CUBEMAP_MIPS; i++ )
	{
		pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &m_prefilteredSpecEnvMapUAV[ i ], nullptr );

		D3D11_MAPPED_SUBRESOURCE mappedSubres;
		pd3dImmediateContext->Map( m_envMapPrefilterCb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );
		Constants consts;
		consts.MipIndex = i;
		consts.MipsNumber = SPEC_CUBEMAP_MIPS;
		memcpy( mappedSubres.pData, &consts, sizeof( Constants ) );
		pd3dImmediateContext->Unmap( m_envMapPrefilterCb, 0 );

		pd3dImmediateContext->CSSetShader( m_envMapSpecPrefilter, nullptr, 0 );
		UINT mipSize = ComputeMipSize( SPEC_CUBEMAP_RESOLUTION, i );
		UINT threadGroupCount = std::max( mipSize / THREAD_GROUP_SIZE, 1u );
		pd3dImmediateContext->Dispatch( threadGroupCount, threadGroupCount, 6 );
	}

	pd3dImmediateContext->CSSetShader( m_brdfLutGen, nullptr, 0 );
	pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &m_brdfLutUav, nullptr );
	pd3dImmediateContext->Dispatch( BRDF_LUT_SIZE / THREAD_GROUP_SIZE, BRDF_LUT_SIZE / THREAD_GROUP_SIZE, 1 );

	pd3dImmediateContext->CSSetShader( m_envMapDiffPrefilter, nullptr, 0 );
	pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &m_prefilteredDiffEnvMapUAV, nullptr );
	UINT threadGroupCount = DIFF_CUBEMAP_RESOLUTION / THREAD_GROUP_SIZE;
	pd3dImmediateContext->Dispatch( threadGroupCount, threadGroupCount, 6 );

	ID3D11ShaderResourceView* nullSrv = nullptr;
	pd3dImmediateContext->CSSetShaderResources( envMapSlot, 1, &nullSrv );

	ID3D11UnorderedAccessView* nullUav = nullptr;
	pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &nullUav, nullptr );
}


void EnvMapFilter::OnD3D11DestroyDevice()
{
	SAFE_RELEASE( m_envMapSpecPrefilter );
	SAFE_RELEASE( m_brdfLutGen );
	SAFE_RELEASE( m_envMapDiffPrefilter );
	SAFE_RELEASE( m_envMapPrefilterCb );

	if( m_prefilteredSpecEnvMapUAV )
	{
		for( UINT i = 0; i < SPEC_CUBEMAP_MIPS; i++ )
			SAFE_RELEASE( m_prefilteredSpecEnvMapUAV[ i ] );

		delete[] m_prefilteredSpecEnvMapUAV;
	}
	SAFE_RELEASE( m_prefilteredSpecEnvMapSRV );
	SAFE_RELEASE( m_prefilteredSpecEnvMap );

	SAFE_RELEASE( m_brdfLutUav );
	SAFE_RELEASE( m_brdfLutSRV );
	SAFE_RELEASE( m_brdfLut );

	SAFE_RELEASE( m_prefilteredDiffEnvMapSRV );
	SAFE_RELEASE( m_prefilteredDiffEnvMapUAV );
	SAFE_RELEASE( m_prefilteredDiffEnvMap );
}


ID3D11ShaderResourceView* EnvMapFilter::GetPrefilteredSpecEnvMap()
{
	return m_prefilteredSpecEnvMapSRV;
}


ID3D11ShaderResourceView* EnvMapFilter::GetBRDFLut()
{
	return m_brdfLutSRV;
}


ID3D11ShaderResourceView* EnvMapFilter::GetPrefilteredDiffEnvMap()
{
	return m_prefilteredDiffEnvMapSRV;
}


HRESULT	EnvMapFilter::ReloadShaders( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	ID3D11ComputeShader* newEnvMapSpecPrefilter = nullptr;
	ID3D11ComputeShader* newBrdfLutGen = nullptr;
	ID3D11ComputeShader* newEnvMapDiffPrefilter = nullptr;

	ID3DBlob* blob = nullptr;
	D3D_SHADER_MACRO macros[ 2 ];
	ZeroMemory( macros, sizeof( macros ) );
	macros[ 0 ].Name = "SPECULAR";
	V_RETURN( CompileShader( L"shaders\\envmapprefilter.hlsl", macros, "cs_main", SHADER_COMPUTE, &blob ) );
	V_RETURN( pd3dDevice->CreateComputeShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &newEnvMapSpecPrefilter ) );
	DXUT_SetDebugName( newEnvMapSpecPrefilter, "EnvMapSpecPrefilter" );

	V_RETURN( CompileShader( L"shaders\\brdflutgen.hlsl", nullptr, "cs_main", SHADER_COMPUTE, &blob ) );
	V_RETURN( pd3dDevice->CreateComputeShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &newBrdfLutGen ) );
	DXUT_SetDebugName( newBrdfLutGen, "BrdfLutGen" );

	macros[ 0 ].Name = "DIFFUSE";
	V_RETURN( CompileShader( L"shaders\\envmapprefilter.hlsl", macros, "cs_main", SHADER_COMPUTE, &blob ) );
	V_RETURN( pd3dDevice->CreateComputeShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &newEnvMapDiffPrefilter ) );
	DXUT_SetDebugName( newEnvMapDiffPrefilter, "EnvMapDiffPrefilter" );

	SAFE_RELEASE( m_envMapSpecPrefilter );
	SAFE_RELEASE( m_brdfLutGen );
	SAFE_RELEASE( m_envMapDiffPrefilter );
	m_envMapSpecPrefilter = newEnvMapSpecPrefilter;
	m_envMapDiffPrefilter = newEnvMapDiffPrefilter;
	m_brdfLutGen = newBrdfLutGen;

	return S_OK;
}
