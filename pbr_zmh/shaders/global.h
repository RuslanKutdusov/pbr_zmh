#pragma pack_matrix( row_major )

cbuffer GlobalParams : register( b0 )
{
	float4x4 ViewProjMatrix;
	float4 ViewPos;
	float4 LightDir;
	float4 LightIrradiance;
	float4x4 ShadowMatrix;
	float IndirectLightIntensity;
	uint FrameIdx;
	uint TotalSamples;
	uint SamplesInStep;
	uint SamplesProcessed;
	bool EnableDirectLight;
	bool EnableIndirectLight;
	bool EnableShadow;
	bool EnableDiffuseLight;
	bool EnableSpecularLight;
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

Texture2D ShadowMap : register( t126 );
TextureCube EnvironmentMap : register( t127 );
SamplerComparisonState CmpLinearSampler : register( s14 );
SamplerState LinearWrapSampler : register( s15 );