#include "Precompiled.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <vector>


HRESULT Model::Load( ID3D11Device* pd3dDevice, const wchar_t* folder, const wchar_t* filename, bool loadMaterials, const TextureMapping& textureMapping )
{
	Assimp::Importer Importer;

	wchar_t relPath[ MAX_PATH ];
	wsprintf( relPath, L"%s\\%s", folder, filename );
	wchar_t fullPath[ MAX_PATH ];
	if( FAILED( DXUTFindDXSDKMediaFileCch( fullPath, MAX_PATH, relPath ) ) )
	{
		ErrorMessageBox( "Sponza model file not found" );
		return E_FAIL;
	}
	char fullPathA[ MAX_PATH ];
	sprintf_s( fullPathA, "%ws", fullPath );

	int flags = aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace ;
	const aiScene* aScene = Importer.ReadFile( fullPathA, flags );

	if( loadMaterials )
	{
		materials = new Material[ aScene->mNumMaterials ];
		for( uint32_t i = 0; i < aScene->mNumMaterials; i++ )
		{
			const UINT printfBufSize = 256;
			aiString aistring;

			aScene->mMaterials[ i ]->Get( AI_MATKEY_NAME, aistring );

			wchar_t albedoPath[ printfBufSize ];
			wchar_t normalPath[ printfBufSize ];
			wchar_t roughnessPath[ printfBufSize ];
			wchar_t metalnessPath[ printfBufSize ];
			memset( albedoPath, 0, printfBufSize * sizeof( wchar_t ) );
			memset( normalPath, 0, printfBufSize * sizeof( wchar_t ) );
			memset( metalnessPath, 0, printfBufSize * sizeof( wchar_t ) );
			memset( roughnessPath, 0, printfBufSize * sizeof( wchar_t ) );

			if( aScene->mMaterials[ i ]->GetTexture( textureMapping[ kTextureDiffuse ], 0, &aistring ) == aiReturn_SUCCESS )
				wsprintf( albedoPath, L"%s\\%S", folder, aistring.C_Str() );

			if( aScene->mMaterials[ i ]->GetTexture( textureMapping[ kTextureNormal ], 0, &aistring ) == aiReturn_SUCCESS )
				wsprintf( normalPath, L"%s\\%S", folder, aistring.C_Str() );

			if( aScene->mMaterials[ i ]->GetTexture( textureMapping[ kTextureMetalness ], 0, &aistring ) == aiReturn_SUCCESS )
				wsprintf( metalnessPath, L"%s\\%S", folder, aistring.C_Str() );

			if( aScene->mMaterials[ i ]->GetTexture( textureMapping[ kTextureRoughness ], 0, &aistring ) == aiReturn_SUCCESS )
				wsprintf( roughnessPath, L"%s\\%S", folder, aistring.C_Str() );

			materials[ i ].Load( pd3dDevice, aistring.C_Str(), albedoPath, normalPath, roughnessPath, metalnessPath );
		}
		materialsNum = aScene->mNumMaterials;
	}
	else
	{
		materials = nullptr;
		materialsNum = 0;
	}

	HRESULT hr = S_OK;
	std::vector< uint32_t > indices;
	indices.resize( 64 * 1024 );
	meshes = new Mesh[ aScene->mNumMeshes ];
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
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &meshes[ i ].posVb ) );
		DXUT_SetDebugName( meshes[ i ].posVb, "posVb" );

		data.pSysMem = mesh->mNormals;
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &meshes[ i ].normalVb ) );
		DXUT_SetDebugName( meshes[ i ].normalVb, "normalVb" );

		data.pSysMem = mesh->mTextureCoords[ 0 ];
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &meshes[ i ].texVb ) );
		DXUT_SetDebugName( meshes[ i ].texVb, "texVb" );

		data.pSysMem = mesh->mTangents;
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &meshes[ i ].tangentVb ) );
		DXUT_SetDebugName( meshes[ i ].tangentVb, "tangentVb" );

		data.pSysMem = mesh->mBitangents;
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &meshes[ i ].binormalVb ) );
		DXUT_SetDebugName( meshes[ i ].binormalVb, "binormalVb" );

		// index buffer
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		uint32_t numIndices = mesh->mNumFaces * 3;
		desc.ByteWidth = sizeof( uint32_t ) * numIndices;
		data.SysMemSlicePitch = data.SysMemPitch;
		if( numIndices > indices.size() )
			indices.resize( numIndices );
		for( uint32_t f = 0; f < mesh->mNumFaces; f++ )
		{
			indices[ f * 3 + 0 ] = mesh->mFaces[ f ].mIndices[ 0 ];
			indices[ f * 3 + 1 ] = mesh->mFaces[ f ].mIndices[ 1 ];
			indices[ f * 3 + 2 ] = mesh->mFaces[ f ].mIndices[ 2 ];
		}
		data.pSysMem = indices.data();
		V_RETURN( pd3dDevice->CreateBuffer( &desc, &data, &meshes[ i ].ib ) );
		DXUT_SetDebugName( meshes[ i ].ib, "ib" );

		meshes[ i ].indexCount = numIndices;
		meshes[ i ].materialIdx = mesh->mMaterialIndex;
	}
	meshesNum = aScene->mNumMeshes;

	return S_OK;
}


void Model::Release()
{
	for( uint32_t i = 0; i < materialsNum; i++ )
		materials[ i ].Release();
	delete[] materials;
	materials = nullptr;

	for( uint32_t i = 0; i < meshesNum; i++ )
	{
		SAFE_RELEASE( meshes[ i ].posVb );
		SAFE_RELEASE( meshes[ i ].normalVb );
		SAFE_RELEASE( meshes[ i ].texVb );
		SAFE_RELEASE( meshes[ i ].tangentVb );
		SAFE_RELEASE( meshes[ i ].binormalVb );
		SAFE_RELEASE( meshes[ i ].ib );
	}
	delete[] meshes;
	meshes = nullptr;
}


void Mesh::Apply( ID3D11DeviceContext* pd3dImmediateContext )
{
	ID3D11Buffer* pVB[ 5 ] = { posVb, normalVb, texVb,
		tangentVb, binormalVb };
	UINT Strides[ 5 ] = { sizeof( aiVector3D ), sizeof( aiVector3D ), sizeof( aiVector3D ),
		sizeof( aiVector3D ), sizeof( aiVector3D ) };
	UINT Offsets[ 5 ] = { 0, 0, 0, 0, 0 };

	pd3dImmediateContext->IASetVertexBuffers( 0, 5, pVB, Strides, Offsets );
	pd3dImmediateContext->IASetIndexBuffer( ib, DXGI_FORMAT_R32_UINT, 0 );
}