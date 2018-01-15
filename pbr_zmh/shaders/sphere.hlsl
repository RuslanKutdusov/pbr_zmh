#include "global.h"
#include "lighting.h"

cbuffer InstanceParams : register( b1 )
{
	float4x4 WorldMatrix;
	float Metalness;
	float Roughness;
};


struct VSOutput
{
	float4 pos : SV_Position;
	float3 worldPos : WORLDPOS;
	float3 normal : NORMAL;
};


VSOutput vs_main( SdkMeshVertex input )
{
	VSOutput output;

	float4 worldPos = mul( float4( input.pos, 1.0f ), WorldMatrix );
	output.pos = mul( worldPos, ViewProjMatrix );

	output.worldPos = worldPos.xyz;
	output.normal = mul( float4( input.norm, 0.0f ), WorldMatrix ).xyz;

	return output;
}


float4 ps_main( VSOutput input ) : SV_Target
{
	float3 normal = normalize( input.normal );
	float3 view = normalize( ViewPos - input.worldPos );
	
	float3 albedo = 1.0f;
	float3 lightColor = 1.0f;
	
	float3 light = CalcLight( normal, LightDir, view, Metalness, Roughness, albedo ) * lightColor;

	float4 ret = float4( light, 1.0f );
	return ret;
}