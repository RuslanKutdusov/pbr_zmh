#include "hammersley.h"

static const float DIELECTRIC_SPEC = 0.04f;
static const float PI = 3.14159265359f;
static const float INV_PI = 0.31830988618f;


float PerceptualRoughnessToRoughness( float perceptualRoughness ) {
	return perceptualRoughness * perceptualRoughness;
}


// тут разные формулы для D, F, G
// http://graphicrants.blogspot.ru/2013/08/specular-brdf-reference.html


// Ref: http://jcgt.org/published/0003/02/03/paper.pdf
// Vis = G / ( 4 * NoL * NoV )
float Vis_SmithJointGGX( float NoL, float NoV, float roughness ) {
	// Approximated version
	float a = roughness;
	float lambdaV = NoL * ( NoV * ( 1.0f - a ) + a );
	float lambdaL = NoV * ( NoL * ( 1.0f - a ) + a );
	return 0.5f * rcp( lambdaV + lambdaL );
}


float D_GGX( float NoH, float roughness ) {
	float a2 = roughness * roughness;
	float d = ( NoH * a2 - NoH ) * NoH + 1.0f; // 2 mad
	return INV_PI * a2 / ( d * d );
}


float3 F_Schlick( float3 F0, float VoH ) {
	float Fc = pow( 1 - VoH, 5 );
	return (1 - Fc) * F0 + Fc;
}


void CalcCdiffAndF0( float3 albedo, float metalness, float reflectance, out float3 cDiff, out float3 F0 )
{
	// albedo для диэлектриков это цвет поверхности, а для
	// металлов это F0.
	// Т.к есть параметр metalness, который равен 0, если поверхность
	// диэлектрик, а если 1 - металл. Он позволяет задать что между
	// диэлектриком и металлом и нам нужно как распределить albedo 
	// между цветом поверхности и F0. 
	// У диэлектриков F0 не бывает меньше ~0.04, но иногда хочется
	// погасить F0 для них еще сильнее, для этого есть параметр reflectance
	float dielectricSpec = DIELECTRIC_SPEC * reflectance;
	float oneMinusReflectivity = ( 1.0f - dielectricSpec ) * ( 1.0f - metalness );
	F0 = lerp( dielectricSpec, albedo, metalness );
	cDiff = albedo * oneMinusReflectivity;
}


// вычисление яркости(radiance) рассеянного(diffuse) и отраженного(specular) света
// от поверхности, освещенной точечным, spot или направленным источником света:
// ( Fd + Fs ) * (n,l) * L * dw
// Fd - diffuse BRDF
// Fs - specular BRDF
// L - radiance от источника света, dw - телесный угол.
// E = L * dw - irradiance от источника.
// для точечных источников или spot light:
//         Ф
// E = -------------
//      4 * pi * r^2
// Ф - поток излучения, Ватт.
// r - расстояние от источника до поверхности
// Для направленных E задается как параметр.
// Функция вычисляет ( Fd + Fs ) * (n,l)
// Итого чтобы вычислить radiance рассеянного(diffuse) и отраженного(specular) света:
// CalcDirectLight(..) * E.
float3 CalcDirectLight( float3 N, float3 L, float3 V, float metalness, float perceptualRoughness, float reflectance, float3 albedo )
{
	float3 cDiff, F0;
	CalcCdiffAndF0( albedo, metalness, reflectance, cDiff, F0 );
	
	float roughness = PerceptualRoughnessToRoughness( perceptualRoughness );
	roughness = max(roughness, 1e-06);

	float3 H = normalize( V + L );
	float NoV = abs( dot( N, V ) ) + 1e-5f;
	float NoL = saturate( dot( N, L ) );
	float NoH = saturate( dot( N, H ) );
	float VoH = saturate( dot( V, H ) );

	// Lambert diffuse
	float3 Fd = cDiff / PI;

	// Micro-facet specular
	// Vis = G / ( 4 * NoL * NoV )
	float Vis = Vis_SmithJointGGX( NoL, NoV, roughness );
	float D = D_GGX( NoH, roughness );
	float3 F = F_Schlick( F0, VoH );
	// D * F * G / ( 4 * NoL * NoV ) = Vis * D * F
	float3 Fs = Vis * D * F;

	float3 sum = 0;
	if( EnableDiffuseLight )
		sum += Fd;
	if( EnableSpecularLight )
		sum += Fs;
	return sum * NoL;
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


// Вычисляет radiance от окружения. Тут численно считается интеграл reflectance equation
// методом Монте-Карло с выборкой по значимости.
float4 CalcIndirectLight( float3 N, float3 V, float metalness, float perceptualRoughness, float reflectance, float3 albedo, uint2 random )
{
	if( SamplesProcessed >= TotalSamples )
		return 0;

	float3 cDiff, F0;
	CalcCdiffAndF0( albedo, metalness, reflectance, cDiff, F0 );

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
			float NoV = abs( dot( N, V ) ) + 1e-5f;
			float NoL = saturate( dot( N, L ) );
			float NoH = saturate( dot( N, H ) );
			float VoH = saturate( dot( V, H ) );
			if( NoL > 0 )
			{
				float pdf = D_GGX( NoH, roughness ) * NoH / (4*VoH);
				
				float solidAngleTexel = 4 * PI / (6 * cubeWidth * cubeWidth);
				float solidAngleSample = 1.0 / (TotalSamples * pdf);
				float lod = roughness == 0 ? 0 : max( 0.5 * log2(solidAngleSample/solidAngleTexel), 0.0f );
				float3 SampleColor = EnvironmentMap.SampleLevel( LinearWrapSampler , L, lod ).rgb;

				float Vis = Vis_SmithJointGGX( NoL, NoV, roughness );
				float3 F = F_Schlick( F0, VoH ); 
				SpecularLighting += SampleColor * F * ( NoL * Vis * (4 * VoH / NoH) );
			}
		}
		// diffuse
		{
			float3 H = ImportanceSampleDiffuse( Xi, N );
			float3 L = normalize( 2 * dot( V, H ) * H - V );
			float NoL = saturate( dot( N, L ) );
			if( NoL > 0 )
			{
				// Compute Lod using inverse solid angle and pdf.
				// From Chapter 20.4 Mipmap filtered samples in GPU Gems 3.
				// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch20.html
				float pdf = NoL * INV_PI;
				
				float solidAngleTexel = 4 * PI / (6 * cubeWidth * cubeWidth);
				float solidAngleSample = 1.0 / (SamplesInStep * pdf);
				float lod = 0.5 * log2((float)(solidAngleSample / solidAngleTexel));

				float3 SampleColor = EnvironmentMap.SampleLevel( LinearWrapSampler, H, lod ).rgb;
				DiffuseLighting += SampleColor;
			}
		}
	}

	float3 sum = 0;
	if( EnableDiffuseLight )
		sum += cDiff * DiffuseLighting;
	if( EnableSpecularLight )
		sum += SpecularLighting;
	return float4( sum, SamplesInStep );
}


static const uint NUM_SAMPLES = 128;
static const uint NUM_MIPS = 9;


float3 PrefilterEnvMap( float roughness, float3 N, float3 V, uint2 random )
{
	uint cubeWidth, cubeHeight;
    EnvironmentMap.GetDimensions(cubeWidth, cubeHeight);
	
	float weight = 0.0f;
	float3 accum = 0.0f;
	for( uint i = 0; i < NUM_SAMPLES; i++ )
	{
		float2 Xi = Hammersley_v1( i, NUM_SAMPLES, random );
		// specular
		{
			float3 H = ImportanceSampleGGX( Xi, roughness, N );
			float3 L = 2 * dot( V, H ) * H - V;
			float NoV = abs( dot( N, V ) ) + 1e-5f;
			float NoL = saturate( dot( N, L ) );
			float NoH = saturate( dot( N, H ) );
			float VoH = saturate( dot( V, H ) );
			if( NoL > 0 )
			{
				float pdf = D_GGX( NoH, roughness ) * NoH / (4*VoH);
				
				float solidAngleTexel = 4 * PI / (6 * cubeWidth * cubeWidth);
				float solidAngleSample = 1.0 / (NUM_SAMPLES * pdf);
				float lod = roughness == 0 ? 0 : max( 0.5 * log2(solidAngleSample/solidAngleTexel), 0.0f );
				
				accum += EnvironmentMap.SampleLevel( LinearWrapSampler , L, lod ).rgb * NoL;
				weight += NoL;
			}
		}
	}
	
	//weight = NUM_SAMPLES;
	return accum / weight;
}


float2 GenerateBRDFLut( float roughness, float NoV, uint2 random )
{
	// Normal always points along z-axis for the 2D lookup 
	const float3 N = float3( 0.0, 0.0, 1.0 );
	float3 V = float3( sqrt( 1.0 - NoV * NoV ), 0.0, NoV );
	
	float2 lut = float2( 0.0, 0.0 );
	for( uint i = 0; i < NUM_SAMPLES; i++ ) 
	{
		float2 Xi = Hammersley_v1( i, NUM_SAMPLES, random );
		float3 H = ImportanceSampleGGX( Xi, roughness, N );
		float3 L = 2 * dot( V, H ) * H - V;
		float NoL = saturate( dot( N, L ) );
		float NoH = saturate( dot( N, H ) );
		float VoH = saturate( dot( V, H ) );
		if( NoL > 0 )
		{
			float Vis = Vis_SmithJointGGX( NoL, NoV, roughness ) * NoL * (4 * VoH / NoH);
			float Fc = pow( 1.0 - VoH, 5.0 );
			
			lut.x += Vis * ( 1.0 - Fc );
			lut.y += Vis * Fc;
		}
	}
	return lut / float(NUM_SAMPLES);
}


float3 GetSpecularDominantDir( float3 N, float3 R, float roughness )
{
	float smoothness = saturate( 1 - roughness );
	float lerpFactor = smoothness * ( sqrt( smoothness ) + roughness );
	// The result is not normalized as we fetch in a cubemap
	return lerp( N, R, lerpFactor );
}


float3 ApproximatedIndirectLight( float3 N, float3 V, float metalness, float perceptualRoughness, float reflectance, float3 albedo, uint2 random )
{
	float3 cDiff, F0;
	CalcCdiffAndF0( albedo, metalness, reflectance, cDiff, F0 );

	float roughness = PerceptualRoughnessToRoughness( perceptualRoughness );
	
	float NoV = abs( dot( N, V ) );
	
	float3 R = 2 * dot( V, N ) * N - V;
	float width, height, numberOfLevels;
	PrefilteredEnvMap.GetDimensions( 0, width, height, numberOfLevels );
	float mip = perceptualRoughness * numberOfLevels; // is the same as sqrt( roughness ) * numberOfLevels
	
	float3 L = 0;
	float2 lut = 0;

	if( ApproxLevel == 0 )
	{
		L = PrefilterEnvMap( roughness, N, V, random );
		lut = GenerateBRDFLut( roughness, NoV, random );
	}
	else if( ApproxLevel == 1 )
	{
		L = PrefilterEnvMap( roughness, R, R, random );
		lut = GenerateBRDFLut( roughness, NoV, random );
	} 
	else if( ApproxLevel == 2 )
	{
		R = GetSpecularDominantDir( N, R, roughness );
		L = PrefilteredEnvMap.SampleLevel( LinearWrapSampler, R, mip ).rgb;
		lut = BRDFLut.Sample( LinearClampSampler, float2( roughness, NoV ) ).xy;
	}
		
	float3 ret = 0;
	if( EnableSpecularLight )
		ret += L * ( F0 * lut.x + lut.y );
	
	return ret;
}


// Тень от направленного источника света
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