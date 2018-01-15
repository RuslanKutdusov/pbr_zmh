#include "global.h"

cbuffer InstanceParams : register( b1 )
{
	float4x4 BakeViewProj[ 6 ];
	bool Bake;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
	uint id : INSTANCE_ID;
};

struct GSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
	uint rtIdx : SV_RenderTargetArrayIndex;
};


Texture2D Texture : register( t0 );
SamplerState Sampler : register( s0 );


VSOutput vs_main( SdkMeshVertex input, uint id : SV_InstanceID )
{
	VSOutput output;

	const float scale = 1.0f;
	if( Bake ) 
		output.pos = mul( float4( input.pos * scale, 1.0f ), BakeViewProj[ id ] );
	else
		output.pos = mul( float4( input.pos * scale + ViewPos, 1.0f ), ViewProjMatrix );

	output.uv = input.tex;
	output.id = id;

	return output;
}

[maxvertexcount( 3 )]
void gs_main( triangle VSOutput input[3], inout TriangleStream< GSOutput > TriStream )
{
	GSOutput output;

	for( int i = 0; i < 3; i++ )
	{
		output.pos = input[ i ].pos;
		output.uv = input[ i ].uv;
		output.rtIdx = input[ i ].id;
		TriStream.Append( output );
	}
	TriStream.RestartStrip();
}


float4 ps_main( GSOutput input ) : SV_Target
{
	return Texture.Sample( Sampler, input.uv );
}