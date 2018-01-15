#include "global.h"

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};


Texture2D Texture : register( t0 );
SamplerState Sampler : register( s0 );


VSOutput vs_main( SdkMeshVertex input )
{
	VSOutput output;

	const float scale = 1.0f;
	output.pos = mul( float4( input.pos * scale + ViewPos, 1.0f ), ViewProjMatrix );

	output.uv = input.tex;

	return output;
}


float4 ps_main( VSOutput input ) : SV_Target
{
	return Texture.Sample( Sampler, input.uv ) / 100.0f;
}