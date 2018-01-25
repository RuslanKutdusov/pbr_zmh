#pragma once


struct RenderTarget
{
	ID3D11Texture2D*			texture = nullptr;
	ID3D11RenderTargetView*		rtv = nullptr;
	ID3D11ShaderResourceView*	srv = nullptr;

	HRESULT Init( ID3D11Device* pd3dDevice, DXGI_FORMAT format, UINT width, UINT height, const char* name );
	void Release();
};
