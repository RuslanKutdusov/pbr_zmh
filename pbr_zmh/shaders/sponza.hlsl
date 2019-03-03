#include "global.h"
#include "lighting.h"

cbuffer InstanceParams : register( b1 )
{
	float3 PointLightPos;
	float3 PointLightFlux;
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
};


VSOutput vs_main( SdkMeshVertex input )
{
	VSOutput output;

	float4 worldPos = float4( input.pos * 0.01f, 1.0f );
	output.pos = mul( worldPos, ViewProjMatrix );

	output.worldPos = worldPos.xyz;
	output.normal = float4( input.normal, 0.0f ).xyz;
	output.tangent = float4( input.tan, 0.0f ).xyz;
	output.binormal = float4( input.binormal, 0.0f ).xyz;
	output.uv = input.tex;

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

	float3 albedo = AlbedoTexture.Sample( LinearWrapSampler, input.uv ).rgb;
	float metalness = MetalnessTexture.Sample( LinearWrapSampler, input.uv ).r;
	float roughness = RoughnessTexture.Sample( LinearWrapSampler, input.uv ).r;
	
	float3 normalTS = NormalTexture.Sample( LinearWrapSampler, input.uv ).rgb * 2.0f - 1.0f;
	normal = normalize( mul( normalTS, float3x3( tangent, binormal, normal ) ) );
	
	PSOutput output = ( PSOutput )0;
	if( EnableDirectLight )
	{
		// Lo = ( Fd + Fs ) * (n,l) * E
		output.directLight.rgb = CalcDirectLight( normal, LightDir.xyz, view, metalness, roughness, 1.0f, albedo ) * LightIrradiance.rgb;
		if( EnableShadow )
			output.directLight.rgb *= CalcShadow( input.worldPos, normalize( input.normal ) );

		float3 lightDir = normalize( PointLightPos.xyz - input.worldPos );
		//         Ф
		// E = -------------
		//      4 * pi * r^2
		float3 irradiance = PointLightFlux.rgb * 1.0f / ( 4.0f * PI * length( PointLightPos.xyz - input.worldPos ) );
		// Lo = ( Fd + Fs ) * (n,l) * E
		output.directLight.rgb += CalcDirectLight( normal, lightDir, view, metalness, roughness, 1.0f, albedo ) * irradiance.rgb;
	}
	if( EnableIndirectLight ) 
	{
		uint2 random = RandVector_v2( pixelPos.xy );
		if( ApproxLevel == APPROX_LEVEL_IS || ApproxLevel == APPROX_LEVEL_FIS )
		{
			output.indirectLight.rgba = CalcIndirectLight( normal, view, metalness, roughness, 1.0f, albedo, random );
		}
		else
		{
			output.directLight.rgb += ApproximatedIndirectLight( normal, view, metalness, roughness, 1.0f, albedo, random ).rgb;
			output.indirectLight.rgb = 0;
			output.indirectLight.a = 0.0;
		}
	}

	return output;
}