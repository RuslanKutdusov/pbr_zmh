#pragma pack_matrix( row_major )

cbuffer GlobalParams : register( b0 )
{
	float4x4 ViewProjMatrix;
	float4 ViewPos;
	float4 LightDir;
	float4 LightIrradiance;
	uint FrameIdx;
	uint TotalSamples;
	uint SamplesInStep;
	uint SamplesProcessed;
	bool EnableDirectLight;
	bool EnableIndirectLight;
	bool2 padding;
};


struct SdkMeshVertex
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXTURE0;
	float3 tan : TANGENT;
	float3 binormal : BINORMAL;
};

TextureCube EnvironmentMap : register( t127 );
SamplerState LinearWrapSampler : register( s15 );