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
	float3 normal : NORMAL;
	uint id : INSTANCE_ID;
};

struct GSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL;
	uint rtIdx : SV_RenderTargetArrayIndex;
};


TextureCube Texture : register( t0 );


VSOutput vs_main( SdkMeshVertex input, uint id : SV_InstanceID )
{
	VSOutput output;

	const float scale = 100.0f;
	if( Bake ) 
		output.pos = mul( float4( input.pos * scale, 1.0f ), BakeViewProj[ id ] );
	else
		output.pos = mul( float4( input.pos * scale + ViewPos, 1.0f ), ViewProjMatrix );

	output.uv = input.tex;
	output.normal = input.norm;
	output.id = id;

	return output;
}

[maxvertexcount( 3 )]
void gs_main( triangle VSOutput input[3], inout TriangleStream< GSOutput > TriStream )
{
	GSOutput output;

	// push in reverse order, cause i am to lasy to create raster state with another cull mode
	for( int i = 2; i >= 0; i-- )
	{
		output.pos = input[ i ].pos;
		output.uv = input[ i ].uv;
		output.normal = input[ i ].normal;
		output.rtIdx = input[ i ].id;
		TriStream.Append( output );
	}
	TriStream.RestartStrip();
}


float4 ps_main( GSOutput input ) : SV_Target
{
	return Texture.Sample( LinearWrapSampler, input.normal );
}