#include "hammersley.h"

static const float DIELECTRIC_SPEC = 0.04f;
static const float PI = 3.14159265359f;
static const float INV_PI = 0.31830988618f;


float PerceptualRoughnessToRoughness( float perceptualRoughness ) {
	return perceptualRoughness * perceptualRoughness;
}


// Ref: http://jcgt.org/published/0003/02/03/paper.pdf
float SmithJointGGXVisibilityTerm( float NoL, float NoV, float roughness ) {
	// Approximised version
	float a = roughness;
	float lambdaV = NoL * ( NoV * ( 1.0f - a ) + a );
	float lambdaL = NoV * ( NoL * ( 1.0f - a ) + a );
	return 0.5f * rcp( lambdaV + lambdaL );
}


float GGXTerm( float NoH, float roughness ) {
	float a2 = roughness * roughness;
	float d = ( NoH * a2 - NoH ) * NoH + 1.0f; // 2 mad
	return INV_PI * a2 / ( d * d );
}


float3 FresnelTerm( float3 F0, float VoH ) {
	float Fc = pow( 1 - VoH, 5 );
	return (1 - Fc) * F0 + Fc;
}


float3 CalcDirectLight( float3 N, float3 L, float3 V, float metalness, float perceptualRoughness, float3 albedo )
{
	float oneMinusReflectivity = ( 1.0f - DIELECTRIC_SPEC ) * ( 1.0f - metalness );
	float3 specularColor = lerp( DIELECTRIC_SPEC, albedo, metalness );
	albedo *= oneMinusReflectivity;
	
	float roughness = PerceptualRoughnessToRoughness( perceptualRoughness );
	roughness = max(roughness, 1e-06);

	float3 H = normalize( V + L );
	float NoV = saturate( dot( N, V ) );
	float NoL = saturate( dot( N, L ) );
	float NoH = saturate( dot( N, H ) );
	float VoH = saturate( dot( V, H ) );

	// Lambert diffuse
	float3 Fd = NoL * albedo / PI;

	// Specular term
	float Vis = SmithJointGGXVisibilityTerm( NoL, NoV, roughness );
	float D = GGXTerm( NoH, roughness );
	float3 F = FresnelTerm( specularColor, VoH );
	float3 Fs = Vis * D * F * NoL;

	return Fd + Fs;
}


float3 ImportanceSampleGGX( float2 E, float Roughness, float3 N )
{
	float m2 = Roughness * Roughness;

	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt( (1 - E.y) / ( 1 + (m2 - 1) * E.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	float3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
	float3 TangentX = normalize( cross( UpVector, N ) );
	float3 TangentY = cross( N, TangentX );
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}


float3 ImportanceSampleDiffuse( float2 Xi, float3 N )
{
    float CosTheta = 1.0f - Xi.y;
    float SinTheta = sqrt( 1.0 - CosTheta * CosTheta );
    float Phi = 2.0f * PI * Xi.x;

    float3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;

    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize( cross( UpVector, N ) );
    float3 TangentY = cross( N, TangentX );

    return TangentX * H.x + TangentY * H.y + N * H.z;
}


float4 CalcIndirectLight( float3 N, float3 V, float metalness, float perceptualRoughness, float3 albedo, uint2 random )
{
	if( SamplesProcessed >= TotalSamples )
		return 0;

	float oneMinusReflectivity = ( 1.0f - DIELECTRIC_SPEC ) * ( 1.0f - metalness );
	float3 specularColor = lerp( DIELECTRIC_SPEC, albedo, metalness );
	albedo *= oneMinusReflectivity;

	float roughness = PerceptualRoughnessToRoughness( perceptualRoughness );

	uint cubeWidth, cubeHeight;
    EnvironmentMap.GetDimensions(cubeWidth, cubeHeight);

	float3 SpecularLighting = 0;
	float3 DiffuseLighting = 0;
	for( uint i = 0; i < SamplesInStep; i++ )
	{
		float2 Xi = Hammersley_v1( i + SamplesProcessed, TotalSamples, random );
		// specular
		{
			float3 H = ImportanceSampleGGX( Xi, roughness, N );
			float3 L = 2 * dot( V, H ) * H - V;
			float NoV = saturate( dot( N, V ) );
			float NoL = saturate( dot( N, L ) );
			float NoH = saturate( dot( N, H ) );
			float VoH = saturate( dot( V, H ) );
			if( NoL > 0 )
			{
				float pdf = GGXTerm( NoH, roughness ) * NoH / (4*VoH);
				
				float solidAngleTexel = 4 * PI / (6 * cubeWidth * cubeWidth);
				float solidAngleSample = 1.0 / (SamplesInStep * pdf);
				float lod = roughness == 0 ? 0 : 0.5 * log2(solidAngleSample/solidAngleTexel);
				float3 SampleColor = EnvironmentMap.SampleLevel( LinearWrapSampler , L, lod ).rgb;

				float Vis = SmithJointGGXVisibilityTerm( NoL, NoV, roughness );
				float3 F = FresnelTerm( specularColor, VoH ); 
				SpecularLighting += SampleColor * F * ( NoL * Vis * (4 * VoH / NoH) );
			}
		}
		// diffuse
		{
			float3 H = ImportanceSampleDiffuse( Xi, N );
			float3 L = normalize( 2 * dot( V, H ) * H - V );
			float NoL = saturate( dot( N, L ) );
			{
				// Compute Lod using inverse solid angle and pdf.
				// From Chapter 20.4 Mipmap filtered samples in GPU Gems 3.
				// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch20.html
				float pdf = max(0.0, dot(N, L) * INV_PI);
				
				float solidAngleTexel = 4 * PI / (6 * cubeWidth * cubeWidth);
				float solidAngleSample = 1.0 / (SamplesInStep * pdf);
				float lod = 0.5 * log2((float)(solidAngleSample / solidAngleTexel));

				float3 SampleColor = EnvironmentMap.SampleLevel( LinearWrapSampler, H, lod ).rgb;
				DiffuseLighting += SampleColor;
			}
		}
	}

	return float4( albedo * DiffuseLighting + SpecularLighting, SamplesInStep );
}



float CalcShadow( float3 worldPos, float3 normal )
{	
	float4 pos = float4( worldPos, 1.0f );
	float3 uv = mul( pos, ShadowMatrix ).xyz;

	float2 shadowMapSize;
    float numSlices;
    ShadowMap.GetDimensions( 0, shadowMapSize.x, shadowMapSize.y, numSlices );
    float texelSize = 2.0f / shadowMapSize.x;
    float nmlOffsetScale = saturate( 1.0f - dot( LightDir.xyz, normal ) );
	float offsetScale = 1.0f;
    float3 normalOffset = texelSize * offsetScale * nmlOffsetScale * normal;
	
	pos = float4( worldPos + normalOffset, 1.0f );
	uv.xyz = mul( pos, ShadowMatrix ).xyz;
	uv.z -= texelSize.x;
	
	return ShadowMap.SampleCmpLevelZero( CmpLinearSampler, uv.xy, uv.z );
}