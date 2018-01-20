#include "global_registers.h"
#pragma pack_matrix( row_major )

cbuffer GlobalParams : register( BRegister( GLOBAL_PARAMS_CB ) )
{
	float4x4 ViewProjMatrix;
	float4 ViewPos;
	float4 LightDir;
	uint FrameIdx;
	uint TotalSamples;
	uint SamplesInStep;
	uint SamplesProcessed;
};


struct SdkMeshVertex
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXTURE0;
};

TextureCube EnvironmentMap : register( TRegister( ENVIRONMENT_MAP ) );
SamplerState LinearWrapSampler : register( SRegister( LINEAR_WRAP_SAMPLER_STATE ) );