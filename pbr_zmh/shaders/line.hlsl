#include "global.h"
#include "lighting.h"

cbuffer Params : register( b1 )
{
	float Metalness;
	float Roughness;
	uint NumSamples;
};


struct VSOutput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
};


VSOutput vs_line( uint vertexId : SV_VertexID )
{
	VSOutput o;
	float3 pos;
	if( vertexId % 2 == 0 )
	{
		pos = 0;
		o.color = float4( 1, 0, 0, 1 );
	}
	else if( vertexId < NumSamples * 2 )
	{
		uint i = vertexId / 2;
		float2 Xi = Hammersley( i, NumSamples, 0 );
		float3 N = float3( 0, 1, 0 );
		//float3 H = ImportanceSampleGGX( Xi, Roughness, N );
		float3 H = ImportanceSampleGGX( Xi, Roughness );
		float3 V = LightDir.xyz;
		float3 L = 2 * dot( V, H ) * H - V;
		float NoL = saturate( dot( N, L ) );
		if( NoL > 0 )
			pos = L;
		else 
			pos = 0;
		pos = L;
		o.color = float4( 1, 0, 0, 0 );
	}
	else if( vertexId == NumSamples * 2 + 1 )
	{
		pos = float3( 0, 1, 0 );
		o.color = float4( 0, 1, 0, 1 );
	}
	else if( vertexId == NumSamples * 2 + 3 )
	{
		pos = LightDir.xyz;
		o.color = float4( 0, 1, 1, 1 );
	}
	
	o.pos = mul( float4( pos, 1.0f ), ViewProjMatrix );	
	return o;
}


VSOutput vs_plane( uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID )
{
	VSOutput o;
	float4 ndc[ 6 ] = {
		float4( -1.0f, 0.0f, -1.0f, 1.0f ),
		float4( -1.0f, 0.0f,  1.0f, 1.0f ),
		float4(  1.0f, 0.0f,  1.0f, 1.0f ),
		float4( -1.0f, 0.0f, -1.0f, 1.0f ),
		float4(  1.0f, 0.0f,  1.0f, 1.0f ),
		float4(  1.0f, 0.0f, -1.0f, 1.0f ),
	};
	
	if( instanceId == 0 )
		o.pos = mul( ndc[ vertexId ], ViewProjMatrix );
	else
		o.pos = mul( ndc[ 5 - vertexId ], ViewProjMatrix );
	
	o.color = float4( 0.1, 0.1, 0.1, 1.0);
	
	return o;
}


float4 ps_main( VSOutput i ) : SV_Target0
{
	return i.color;
}