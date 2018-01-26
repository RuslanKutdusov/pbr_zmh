#pragma once
#include <assimp/material.h>     


enum TextureType
{
	kTextureDiffuse = 0,
	kTextureNormal,
	kTextureRoughness,
	kTextureMetalness,

	kTextureTypeCount
};


struct TextureMapping
{
	aiTextureType mapping[ kTextureTypeCount ];

	TextureMapping()
	{
		memset( mapping, 0, kTextureTypeCount * sizeof( aiTextureType ) );
	}

	const aiTextureType& operator[]( const TextureType t ) const
	{
		return mapping[ t ];
	}

	aiTextureType& operator[]( const TextureType t ) 
	{
		return mapping[ t ];
	}
};


struct Mesh
{
	ID3D11Buffer* posVb = nullptr;
	ID3D11Buffer* normalVb = nullptr;
	ID3D11Buffer* texVb = nullptr;
	ID3D11Buffer* tangentVb = nullptr;
	ID3D11Buffer* binormalVb = nullptr;
	ID3D11Buffer* ib = nullptr;
	uint32_t materialIdx = 0;
	uint32_t indexCount = 0;

	void Apply( ID3D11DeviceContext* pd3dImmediateContext );
};


struct Model
{
	HRESULT Load( ID3D11Device* pd3dDevice, const wchar_t* folder, const wchar_t* filename, bool loadMaterials, const TextureMapping& textureMapping );
	void Release();

	Material* materials = nullptr;
	uint32_t materialsNum = 0;

	Mesh* meshes = nullptr;
	uint32_t meshesNum = 0;
};
