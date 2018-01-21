#include "global.h"
#include "lighting.h"

cbuffer InstanceParams : register( b1 )
{
	struct
	{
		float4x4 WorldMatrix;
		float Metalness;
		float Roughness;
		bool EnableDirectLight;
		bool EnableIndirectLight;
		bool UseMaterial;
		bool3 padding;
	} InstanceData[ 128 ];
};
Texture2D AlbedoTexture : register( t0 );
Texture2D NormalTexture : register( t1 );
Texture2D RoughnessTexture : register( t2 );
Texture2D MetalnessTexture : register( t3 );


struct VSOutput
{
	float4 pos : SV_Position;
	float3 worldPos : WORLDPOS;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : UV;
	uint id : INSTANCE_ID;
};


VSOutput vs_main( SdkMeshVertex input, uint id : SV_InstanceID )
{
	VSOutput output;

	float4 worldPos = mul( float4( input.pos, 1.0f ), InstanceData[ id ].WorldMatrix );
	output.pos = mul( worldPos, ViewProjMatrix );

	output.worldPos = worldPos.xyz;
	output.normal = mul( float4( input.normal, 0.0f ), InstanceData[ id ].WorldMatrix ).xyz;
	output.tangent = mul( float4( input.tan, 0.0f ), InstanceData[ id ].WorldMatrix ).xyz;
	output.binormal = mul( float4( input.binormal, 0.0f ), InstanceData[ id ].WorldMatrix ).xyz;
	output.uv = input.tex;

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
	float3 normal = normalize( input.normal );
	float3 tangent = normalize( input.tangent );
	float3 binormal = normalize( input.binormal );//cross( normal, tangent );
	float3 view = normalize( ViewPos.xyz - input.worldPos );

	float metalness = 0.0f;
	float roughness = 0.0f;
	float3 albedo = 1.0f;
	if( InstanceData[ input.id ].UseMaterial )
	{
		albedo = pow( AlbedoTexture.Sample( LinearWrapSampler, input.uv ), 2.2f );
		metalness = MetalnessTexture.Sample( LinearWrapSampler, input.uv );
		roughness = RoughnessTexture.Sample( LinearWrapSampler, input.uv );

		float3 normalTS = NormalTexture.Sample( LinearWrapSampler, input.uv ) * 2.0f - 1.0f;
		normal = normalize( mul( normalTS, float3x3( tangent, binormal, normal ) ) );
	}
	else
	{
		metalness = InstanceData[ input.id ].Metalness;
		roughness = InstanceData[ input.id ].Roughness;
	}
	bool enableDirectLight = InstanceData[ input.id ].EnableDirectLight;
	bool enableIndirectLight = InstanceData[ input.id ].EnableIndirectLight;
	
	float3 lightColor = 1.0f;
	
	PSOutput output = ( PSOutput )0;
	if( enableDirectLight )
		output.directLight.rgb = CalcDirectLight( normal, LightDir.xyz, view, metalness, roughness, albedo ) * lightColor;
	if( enableIndirectLight ) {
		uint2 random = RandVector_v2( pixelPos.xy );
		output.indirectLight.rgba = CalcIndirectLight( normal, view, metalness, roughness, albedo, random );
	}

	return output;
}