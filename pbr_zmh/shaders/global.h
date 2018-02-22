#pragma pack_matrix( row_major )

cbuffer GlobalParams : register( b0 )
{
	float4x4 ViewProjMatrix;
	float4 ViewPos;
	float4 LightDir;
	float4 LightIrradiance;
	float4x4 ShadowMatrix;
	float IndirectLightIntensity;
	uint ApproxLevel;
	uint FrameIdx;
	uint TotalSamples;
	uint SamplesInStep;
	uint SamplesProcessed;
	bool EnableDirectLight;
	bool EnableIndirectLight;
	bool EnableShadow;
	bool EnableDiffuseLight;
	bool EnableSpecularLight;
	uint ScreenWidth;
	uint ScreenHeight;
};


struct SdkMeshVertex
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXTURE0;
	float3 tan : TANGENT;
	float3 binormal : BINORMAL;
};

TextureCube PrefilteredDiffuseEnvMap: register( t123 );
Texture2D BRDFLut : register( t124 );
TextureCube PrefilteredSpecularEnvMap: register( t125 );
Texture2D ShadowMap : register( t126 );
TextureCube EnvironmentMap : register( t127 );
SamplerState LinearClampSampler : register( s13 );
SamplerComparisonState CmpLinearSampler : register( s14 );
SamplerState LinearWrapSampler : register( s15 );