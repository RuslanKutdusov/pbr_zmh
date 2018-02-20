#include "Precompiled.h"


const UINT CUBEMAP_RESOLUTION = 256;
const UINT CUBEMAP_MIPS = 9;
const UINT BRDF_LUT_SIZE = 256;


struct Constants
{
	UINT FaceIndex;
	float TargetRoughness;
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
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &m_cb ) );
	DXUT_SetDebugName( m_cb, "Constants" );

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory( &texDesc, sizeof( D3D11_TEXTURE2D_DESC ) );
	texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;

	// prefiltered env map
	texDesc.ArraySize = 6;
	texDesc.Width = CUBEMAP_RESOLUTION;
	texDesc.Height = CUBEMAP_RESOLUTION;
	texDesc.MipLevels = CUBEMAP_MIPS;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, nullptr, &m_prefilteredEnvMap ) );
	DXUT_SetDebugName( m_prefilteredEnvMap, "PrefilteredEnvMap" );

	m_prefilteredEnvMapUAV = new ID3D11UnorderedAccessView*[ CUBEMAP_MIPS ];
	for( UINT i = 0; i < CUBEMAP_MIPS; i++ )
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory( &uavDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
		uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = 6;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = i;

		V_RETURN( pd3dDevice->CreateUnorderedAccessView( m_prefilteredEnvMap, &uavDesc, &m_prefilteredEnvMapUAV[ i ] ) );
	}

	// BRDF Lut
	texDesc.ArraySize = 1;
	texDesc.Width = BRDF_LUT_SIZE;
	texDesc.Height = texDesc.Width;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, nullptr, &m_brdfLut ) );
	DXUT_SetDebugName( m_brdfLut, "BRDF Lut" );

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory( &uavDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
	uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateUnorderedAccessView( m_brdfLut, &uavDesc, &m_brdfLutUav ) );

	return S_OK;
}


void EnvMapFilter::FilterEnvMap( ID3D11DeviceContext* pd3dImmediateContext, UINT envMapSlot, ID3D11ShaderResourceView* envmap )
{
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Filter env map" );
	pd3dImmediateContext->CSSetShader( m_cs, nullptr, 0 );
	pd3dImmediateContext->CSSetConstantBuffers( 1, 1, &m_cb );
	pd3dImmediateContext->CSSetShaderResources( envMapSlot, 1, &envmap );

	for( UINT i = 0; i < CUBEMAP_MIPS; i++ )
	{
		UINT mipSize = ComputeMipSize( CUBEMAP_RESOLUTION, i );
		UINT threadGroupCount = std::max( mipSize / 32, 1u );

		float clearValues[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pd3dImmediateContext->ClearUnorderedAccessViewFloat( m_prefilteredEnvMapUAV[ i ], clearValues );
		pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &m_prefilteredEnvMapUAV[ i ], nullptr );

		for( UINT f = 0; f < 6; f++ )
		{
			D3D11_MAPPED_SUBRESOURCE mappedSubres;
			pd3dImmediateContext->Map( m_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );
			Constants consts;
			consts.FaceIndex = f;
			consts.TargetRoughness = ( float )i / ( ( float )CUBEMAP_MIPS - 1.0f );
			memcpy( mappedSubres.pData, &consts, sizeof( Constants ) );
			pd3dImmediateContext->Unmap( m_cb, 0 );

			pd3dImmediateContext->Dispatch( threadGroupCount, threadGroupCount, 1 );
		}
	}

	ID3D11ShaderResourceView* nullSrv = nullptr;
	pd3dImmediateContext->CSSetShaderResources( envMapSlot, 1, &nullSrv );
	DXUT_EndPerfEvent();
}


void EnvMapFilter::OnD3D11DestroyDevice()
{
	SAFE_RELEASE( m_cs );
	SAFE_RELEASE( m_cb );

	if( m_prefilteredEnvMapUAV )
	{
		for( UINT i = 0; i < CUBEMAP_MIPS; i++ )
			SAFE_RELEASE( m_prefilteredEnvMapUAV[ i ] );

		delete[] m_prefilteredEnvMapUAV;
	}
	SAFE_RELEASE( m_prefilteredEnvMap );

	SAFE_RELEASE( m_brdfLutUav );
	SAFE_RELEASE( m_brdfLut );
}



HRESULT	EnvMapFilter::ReloadShaders( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	ID3D11ComputeShader* newCs = nullptr;

	ID3DBlob* blob = nullptr;
	V_RETURN( CompileShader( L"shaders\\envmapfilter.hlsl", nullptr, "cs_main", SHADER_COMPUTE, &blob ) );
	V_RETURN( pd3dDevice->CreateComputeShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &newCs ) );
	DXUT_SetDebugName( newCs, "SkyVS" );

	SAFE_RELEASE( m_cs );
	m_cs = newCs;

	return S_OK;
}
