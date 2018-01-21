#include "global_registers.h"
#pragma pack_matrix( row_major )

cbuffer GlobalParams : register( BRegister( GLOBAL_PARAMS_CB ) )
{
	float4x4 ViewProjMatrix;
	float4 ViewPos;
	float4 LightDir;
	float4 LightIrradiance;
	uint FrameIdx;
	uint TotalSamples;
	uint SamplesInStep;
	uint SamplesProcessed;
};


struct SdkMeshVertex
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXTURE0;
	float3 tan : TANGENT;
	float3 binormal : BINORMAL;
};

TextureCube EnvironmentMap : register( TRegister( ENVIRONMENT_MAP ) );
SamplerState LinearWrapSampler : register( SRegister( LINEAR_WRAP_SAMPLER_STATE ) );