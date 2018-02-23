#include "global.h"
#include "lighting.h"

#define MATERIAL_SIMPLE 0
#define MATERIAL_TEXTURE 1
#define MATERIAL_MERL 2

cbuffer InstanceParams : register( b1 )
{
	struct
	{
		float4x4 WorldMatrix;
		float Metalness;
		float Roughness;
		float Reflectance;
		float4 Albedo;
		uint MaterialType;
		uint3 padding;
	} InstanceData[ 128 ];
};
Texture2D AlbedoTexture : register( t0 );
Texture2D NormalTexture : register( t1 );
Texture2D RoughnessTexture : register( t2 );
Texture2D MetalnessTexture : register( t3 );
Buffer<float> MerlBRDF : register( t4 );

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
	float reflectance = 1.0f;
	float3 albedo = 1.0f;
	if( InstanceData[ input.id ].MaterialType == MATERIAL_SIMPLE )
	{
		albedo = InstanceData[ input.id ].Albedo.rgb;
		metalness = InstanceData[ input.id ].Metalness;
		roughness = InstanceData[ input.id ].Roughness;
		reflectance = InstanceData[ input.id ].Reflectance;
	}
	else if( InstanceData[ input.id ].MaterialType == MATERIAL_TEXTURE )
	{
		albedo = AlbedoTexture.Sample( LinearWrapSampler, input.uv ).rgb;
		metalness = MetalnessTexture.Sample( LinearWrapSampler, input.uv ).r;
		roughness = RoughnessTexture.Sample( LinearWrapSampler, input.uv ).r;

		float3 normalTS = NormalTexture.Sample( LinearWrapSampler, input.uv ).rgb * 2.0f - 1.0f;
		normal = normalize( mul( normalTS, float3x3( tangent, binormal, normal ) ) );
	}
	
	PSOutput output = ( PSOutput )0;
	if( InstanceData[ input.id ].MaterialType == MATERIAL_MERL )
	{
		output.directLight.rgb = CalcDirectLight( MerlBRDF, LightDir.xyz, view, normal, tangent, binormal ) * LightIrradiance.rgb;
	}
	else
	{
		if( EnableDirectLight )
		{
			// Lo = ( Fd + Fs ) * (n,l) * E
			output.directLight.rgb = CalcDirectLight( normal, LightDir.xyz, view, metalness, roughness, reflectance, albedo ) * LightIrradiance.rgb;
			if( EnableShadow )
				output.directLight.rgb *= CalcShadow( input.worldPos, normalize( input.normal ) );
		}
		if( EnableIndirectLight ) 
		{
			uint2 random = RandVector_v2( pixelPos.xy );
			output.indirectLight.rgba = CalcIndirectLight( normal, view, metalness, roughness, reflectance, albedo, random );
			/*output.directLight.rgb += ApproximatedIndirectLight( normal, view, metalness, roughness, reflectance, albedo, random ).rgb;
			output.indirectLight.rgb = 0;
			output.indirectLight.a = 0.0;*/
		}
	}

	return output;
}