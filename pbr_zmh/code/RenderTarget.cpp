#include "Precompiled.h"


HRESULT RenderTarget::Init(ID3D11Device* pd3dDevice, DXGI_FORMAT format, UINT width, UINT height, const char* name )
{
	HRESULT hr;
	Release();

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = format;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	V_RETURN( pd3dDevice->CreateTexture2D( &texDesc, nullptr, &texture ) );
	DXUT_SetDebugName( texture, name );

	// Create the render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
	rtDesc.Format = texDesc.Format;
	rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtDesc.Texture2D.MipSlice = 0;
	V_RETURN( pd3dDevice->CreateRenderTargetView( texture, &rtDesc, &rtv ) );
	DXUT_SetDebugName(rtv, name );

	// Create the shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	V_RETURN(pd3dDevice->CreateShaderResourceView( texture, &srvDesc, &srv ) );
	DXUT_SetDebugName( srv, name );

	return S_OK;
}



void RenderTarget::Release()
{
	SAFE_RELEASE(texture);
	SAFE_RELEASE(rtv);
	SAFE_RELEASE(srv);
}