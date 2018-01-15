#include "Precompiled.h"

using namespace DirectX;


struct Params
{
	float Exposure;
	float padding[ 3 ];
};


PostProcess::PostProcess()
{

}


HRESULT PostProcess::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	WCHAR str[ MAX_PATH ];
	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"shaders\\quad.vs" ) );

	ID3DBlob* blob = nullptr;
	V_RETURN( DXUTCompileFromFile( str, nullptr, "vs_main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG, 0, &blob ) );
	V_RETURN( pd3dDevice->CreateVertexShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_quadVs ) );
	DXUT_SetDebugName( m_quadVs, "QuadVS" );
	SAFE_RELEASE( blob );

	V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"shaders\\postprocess.ps" ) );

	V_RETURN( DXUTCompileFromFile( str, nullptr, "Tonemap", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG, 0, &blob ) );
	V_RETURN( pd3dDevice->CreatePixelShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_tonemapPs ) );
	DXUT_SetDebugName( m_tonemapPs, "TonemapPS" );
	SAFE_RELEASE( blob );

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

	D3D11_DEPTH_STENCIL_DESC depthDesc;
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDesc.StencilEnable = FALSE;
	depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS };
	depthDesc.FrontFace = defaultStencilOp;
	depthDesc.BackFace = defaultStencilOp;
	pd3dDevice->CreateDepthStencilState( &depthDesc, &m_depthStencilState );

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof( Params );
	V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &m_paramsBuf ) );
	DXUT_SetDebugName( m_paramsBuf, "PostProcessParams" );

	return S_OK;
}


void PostProcess::Render( ID3D11DeviceContext* pd3dImmediateContext, ID3D11ShaderResourceView* hdrTextureSRV, float exposure )
{
	pd3dImmediateContext->IASetInputLayout( nullptr );
	pd3dImmediateContext->VSSetShader( m_quadVs, nullptr, 0 );
	pd3dImmediateContext->PSSetShader( m_tonemapPs, nullptr, 0 );

	pd3dImmediateContext->PSSetShaderResources( 0, 1, &hdrTextureSRV );
	pd3dImmediateContext->PSSetSamplers( 0, 1, &m_samperState );

	ID3D11DepthStencilState* savedDepthStencilState;
	UINT savedStencilRef;
	pd3dImmediateContext->OMGetDepthStencilState( &savedDepthStencilState, &savedStencilRef );
	pd3dImmediateContext->OMSetDepthStencilState( m_depthStencilState, 0 );

	{
		D3D11_MAPPED_SUBRESOURCE mappedSubres;
		pd3dImmediateContext->Map( m_paramsBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres );

		Params* params = ( Params* )mappedSubres.pData;
		params->Exposure = exposure;
		pd3dImmediateContext->Unmap( m_paramsBuf, 0 );

		// apply
		pd3dImmediateContext->VSSetConstantBuffers( 1, 1, &m_paramsBuf );
		pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_paramsBuf );
	}

	pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	pd3dImmediateContext->Draw( 6, 0 );

	// restore depth stencil state
	pd3dImmediateContext->OMSetDepthStencilState( savedDepthStencilState, savedStencilRef );
	savedDepthStencilState->Release();
}


void PostProcess::OnD3D11DestroyDevice()
{
	SAFE_RELEASE( m_quadVs );
	SAFE_RELEASE( m_tonemapPs );
	SAFE_RELEASE( m_samperState );
	SAFE_RELEASE( m_depthStencilState );
}