#include "Precompiled.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <vector>


SponzaRenderer::SponzaRenderer()
{

}


HRESULT SponzaRenderer::OnD3D11CreateDevice( ID3D11Device* pd3dDevice )
{
	Assimp::Importer Importer;

	WCHAR fullPath[ MAX_PATH ];
	if( FAILED( DXUTFindDXSDKMediaFileCch( fullPath, MAX_PATH, L"sponza\\sponza.obj" ) ) )
	{
		ErrorMessageBox( "Sponza model file not found" );
		return E_FAIL;
	}
	char fullPathA[ MAX_PATH ];
	sprintf_s( fullPathA, "%ws", fullPath );

	int flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
	const aiScene* aScene = Importer.ReadFile( fullPathA, flags );

	m_materials = new Material[ aScene->mNumMaterials ];
	for( uint32_t i = 0; i < aScene->mNumMaterials; i++ )
	{
		const UINT printfBufSize = 256;
		aiString aistring;

		wchar_t materialName[ printfBufSize ];
		aScene->mMaterials[ i ]->Get( AI_MATKEY_NAME, aistring );
		wsprintf( materialName, L"%S", aistring.C_Str() );

		wchar_t albedoPath[ printfBufSize ];
		wchar_t normalPath[ printfBufSize ];
		wchar_t roughnessPath[ printfBufSize ];
		wchar_t metalnessPath[ printfBufSize ];
		memset( albedoPath, 0, printfBufSize * sizeof( wchar_t ) );
		memset( normalPath, 0, printfBufSize * sizeof( wchar_t ) );
		memset( metalnessPath, 0, printfBufSize * sizeof( wchar_t ) );
		memset( roughnessPath, 0, printfBufSize * sizeof( wchar_t ) );

		if( aScene->mMaterials[ i ]->GetTexture( aiTextureType_DIFFUSE, 0, &aistring ) == aiReturn_SUCCESS )
			wsprintf( albedoPath, L"sponza\\%S", aistring.C_Str() );

		if( aScene->mMaterials[ i ]->GetTexture( aiTextureType_HEIGHT, 0, &aistring ) == aiReturn_SUCCESS )
			wsprintf( normalPath, L"sponza\\%S", aistring.C_Str() );

		if( aScene->mMaterials[ i ]->GetTexture( aiTextureType_AMBIENT, 0, &aistring ) == aiReturn_SUCCESS )
			wsprintf( metalnessPath, L"sponza\\%S", aistring.C_Str() );

		if( aScene->mMaterials[ i ]->GetTexture( aiTextureType_SHININESS, 0, &aistring ) == aiReturn_SUCCESS )
			wsprintf( roughnessPath, L"sponza\\%S", aistring.C_Str() );

		m_materials[ i ].Load( pd3dDevice, materialName, albedoPath, normalPath, roughnessPath, metalnessPath );
	}
	m_materialsNum = aScene->mNumMaterials;

	HRESULT hr = S_OK;
	std::vector< uint32_t > indices;
	indices.resize( 64 * 1024 );
	m_meshes = new Mesh[ aScene->mNumMeshes ];
	for( uint32_t i = 0; i < aScene->mNumMeshes; i++ )
	{
		aiMesh* mesh = aScene->mMeshes[ i ];

		D3D11_BUFFER_DESC desc;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA data;
		
		// vertex buffers
		desc.ByteWidth = sizeof( aiVector3D ) * mesh->mNumVertices;
		data.SysMemPitch = desc.ByteWidth;
		data.SysMemSlicePitch = data.SysMemPitch;
		data.pSysMem = mesh->mVertices;
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &m_meshes[ i ].posVb ) );
		DXUT_SetDebugName( m_meshes[ i ].posVb, "posVb" );

		data.pSysMem = mesh->mNormals;
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &m_meshes[ i ].normalVb ) );
		DXUT_SetDebugName( m_meshes[ i ].normalVb, "normalVb" );

		data.pSysMem = mesh->mTextureCoords[ 0 ];
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &m_meshes[ i ].texVb ) );
		DXUT_SetDebugName( m_meshes[ i ].texVb, "texVb" );

		data.pSysMem = mesh->mTangents;
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &m_meshes[ i ].tangentVb ) );
		DXUT_SetDebugName( m_meshes[ i ].tangentVb, "tangentVb" );

		data.pSysMem = mesh->mBitangents;
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &m_meshes[ i ].binormalVb ) );
		DXUT_SetDebugName( m_meshes[ i ].binormalVb, "binormalVb" );

		// index buffer
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		uint32_t numIndices = mesh->mNumFaces * 3;
		desc.ByteWidth = sizeof( uint32_t ) * numIndices;
		data.SysMemSlicePitch = data.SysMemPitch;
		if( numIndices > indices.size() )
			indices.resize( numIndices );
		for( uint32_t f = 0; f < mesh->mNumFaces; f++ )
		{
			indices[ f * 3 + 0 ] = mesh->mFaces[ f ].mIndices[ 2 ];
			indices[ f * 3 + 1 ] = mesh->mFaces[ f ].mIndices[ 1 ];
			indices[ f * 3 + 2 ] = mesh->mFaces[ f ].mIndices[ 0 ];
		}
		data.pSysMem = indices.data();
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &m_meshes[ i ].ib ) );
		DXUT_SetDebugName( m_meshes[ i ].ib, "ib" );

		m_meshes[ i ].indexCount = numIndices;
		m_meshes[ i ].materialIdx = mesh->mMaterialIndex;
	}
	m_meshesNum = aScene->mNumMeshes;

	V_RETURN( ReloadShaders( pd3dDevice ) );

	return S_OK;
}


void SponzaRenderer::Render( ID3D11DeviceContext* pd3dImmediateContext, bool depthPass )
{
	pd3dImmediateContext->IASetInputLayout( m_inputLayout );
	pd3dImmediateContext->VSSetShader( m_vs, nullptr, 0 );
	pd3dImmediateContext->PSSetShader( depthPass ? nullptr : m_ps, nullptr, 0 );

	for( uint32_t i = 0; i < m_meshesNum; i++ )
	{
		Mesh& mesh = m_meshes[ i ];

		ID3D11Buffer* pVB[ 5 ] = { m_meshes[ i ].posVb, m_meshes[ i ].normalVb, m_meshes[ i ].texVb,
			m_meshes[ i ].tangentVb, m_meshes[ i ].binormalVb };
		UINT Strides[ 5 ] = { sizeof( aiVector3D ), sizeof( aiVector3D ), sizeof( aiVector3D ),
			sizeof( aiVector3D ), sizeof( aiVector3D ) };
		UINT Offsets[ 5 ] = { 0, 0, 0, 0, 0 };
		pd3dImmediateContext->IASetVertexBuffers( 0, 5, pVB, Strides, Offsets );
		pd3dImmediateContext->IASetIndexBuffer( m_meshes[ i ].ib, DXGI_FORMAT_R32_UINT, 0 );

		if( !depthPass )
		{
			Material& m = m_materials[ mesh.materialIdx ];
			pd3dImmediateContext->PSSetShaderResources( 0, 1, &m.albedo );
			pd3dImmediateContext->PSSetShaderResources( 1, 1, &m.normal );
			pd3dImmediateContext->PSSetShaderResources( 2, 1, &m.roughness );
			pd3dImmediateContext->PSSetShaderResources( 3, 1, &m.metalness );
		}

		pd3dImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		pd3dImmediateContext->DrawIndexed( m_meshes[ i ].indexCount, 0, 0 );
	}
}


void SponzaRenderer::RenderDepthPass( ID3D11DeviceContext* pd3dImmediateContext )
{
	Render( pd3dImmediateContext, true );
}


void SponzaRenderer::RenderLightPass( ID3D11DeviceContext* pd3dImmediateContext )
{
	Render( pd3dImmediateContext, false );
}


void SponzaRenderer::OnD3D11DestroyDevice()
{
	for( uint32_t i = 0; i < m_materialsNum; i++ )
		m_materials[ i ].Release();
	delete[] m_materials;
	m_materials = nullptr;

	for( uint32_t i = 0; i < m_meshesNum; i++ )
	{
		SAFE_RELEASE( m_meshes[ i ].posVb );
		SAFE_RELEASE( m_meshes[ i ].normalVb );
		SAFE_RELEASE( m_meshes[ i ].texVb );
		SAFE_RELEASE( m_meshes[ i ].tangentVb );
		SAFE_RELEASE( m_meshes[ i ].binormalVb );
		SAFE_RELEASE( m_meshes[ i ].ib );
	}
	delete[] m_meshes;
	m_meshes = nullptr;

	SAFE_RELEASE( m_inputLayout );
	SAFE_RELEASE( m_vs );
	SAFE_RELEASE( m_ps );
}


HRESULT	SponzaRenderer::ReloadShaders( ID3D11Device* pd3dDevice )
{
	HRESULT hr;
	ID3D11VertexShader* newVs = nullptr;
	ID3D11PixelShader* newPs = nullptr;
	ID3D11InputLayout* newInputLayout = nullptr;

	ID3DBlob* vsBlob = nullptr;
	V_RETURN( CompileShader( L"shaders\\sponza.hlsl", nullptr, "vs_main", SHADER_VERTEX, &vsBlob ) );
	V_RETURN( pd3dDevice->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &newVs ) );
	DXUT_SetDebugName( newVs, "SponzaVS" );

	ID3DBlob* psBlob = nullptr;
	V_RETURN( CompileShader( L"shaders\\sponza.hlsl", nullptr, "ps_main", SHADER_PIXEL, &psBlob ) );
	V_RETURN( pd3dDevice->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &newPs ) );
	DXUT_SetDebugName( newPs, "SponzaPS" );
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


