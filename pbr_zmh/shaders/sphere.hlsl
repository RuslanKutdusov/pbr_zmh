#include "global.h"
#include "lighting.h"

cbuffer InstanceParams : register( b1 )
{
	struct
	{
		float4x4 WorldMatrix;
		float Metalness;
		float Roughness;
		bool DirectLight;
		bool IndirectLight;
	} InstanceData[ 128 ];
};


struct VSOutput
{
	float4 pos : SV_Position;
	float3 worldPos : WORLDPOS;
	float3 normal : NORMAL;
	uint id : INSTANCE_ID;
};


VSOutput vs_main( SdkMeshVertex input, uint id : SV_InstanceID )
{
	VSOutput output;

	float4 worldPos = mul( float4( input.pos, 1.0f ), InstanceData[ id ].WorldMatrix );
	output.pos = mul( worldPos, ViewProjMatrix );

	output.worldPos = worldPos.xyz;
	output.normal = mul( float4( input.norm, 0.0f ), InstanceData[ id ].WorldMatrix ).xyz;

	output.id = id;

	return output;
}


struct PSOutput
{
	float4 directLight : SV_Target0;
	float4 indirectLight : SV_Target1;
};


PSOutput ps_main( VSOutput input, float4 pixelPos : SV_Position )
{
	float metalness = InstanceData[ input.id ].Metalness;
	float roughness = InstanceData[ input.id ].Roughness;
	float directLight = InstanceData[ input.id ].DirectLight;
	float indirectLight = InstanceData[ input.id ].IndirectLight;

	float3 normal = normalize( input.normal );
	float3 view = normalize( ViewPos.xyz - input.worldPos );
	
	float3 albedo = 1.0f;
	float3 lightColor = 1.0f;
	
	PSOutput output = ( PSOutput )0;
	if( directLight )
		output.directLight.rgb = CalcLight( normal, LightDir.xyz, view, metalness, roughness, albedo ) * lightColor;
	if( indirectLight ) {
		uint2 random = RandVector_v2( pixelPos.xy );
		output.indirectLight.rgba = GetEnvironmentLight( normal, view, metalness, roughness, albedo, random );
	}

	return output;
}