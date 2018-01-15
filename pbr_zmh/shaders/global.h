#pragma pack_matrix( row_major )

cbuffer GlobalParams : register( b0 )
{
	float4x4 ViewProjMatrix;
	float3 ViewPos;
	float3 LightDir;
};


struct SdkMeshVertex
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXTURE0;
};