#include "global.h"
#include "lighting.h"

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
		output.directLight.rgb = CalcDirectLight( normal, LightDir.xyz, view, metalness, roughness, albedo ) * LightIrradiance.rgb;
	if( EnableIndirectLight ) 
	{
		uint2 random = RandVector_v2( pixelPos.xy );
		output.indirectLight.rgba = CalcIndirectLight( normal, view, metalness, roughness, albedo, random );
	}

	return output;
}