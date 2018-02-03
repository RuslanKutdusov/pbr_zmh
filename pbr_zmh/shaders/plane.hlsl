#include "global.h"
#include "lighting.h"

cbuffer InstanceParams : register( b1 )
{
	struct
	{
		float4x4 WorldMatrix;
	} InstanceData[ 2 ];
};
Texture2D MetalnessTexture : register( t0 );
Texture2D RoughnessTexture : register( t1 );


struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : UV;
	uint id : INSTANCE_ID;
};


VSOutput vs_main( SdkMeshVertex input, uint id : SV_InstanceID )
{
	VSOutput output;

	float4 worldPos = mul( float4( input.pos, 1.0f ), InstanceData[ id ].WorldMatrix );
	output.pos = mul( worldPos, ViewProjMatrix );
	output.uv = input.tex;
	output.id = id;

	return output;
}


struct PSOutput
{
	float4 directLight : SV_Target0;
	float4 indirectLight : SV_Target1;
};


PSOutput ps_main( VSOutput input )
{
	PSOutput output = ( PSOutput )0;
	if( input.id == 0 )
		output.directLight = MetalnessTexture.Sample( LinearWrapSampler, input.uv );
	else
		output.directLight = RoughnessTexture.Sample( LinearWrapSampler, input.uv );

	return output;
}