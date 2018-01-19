static const float DIELECTRIC_SPEC = 0.04f;
static const float PI = 3.14159265359f;
static const float INV_PI = 0.31830988618f;

// Pow5 uses the same amount of instructions as generic pow(), but has 2 advantages:
// 1) better instruction pipelining
// 2) no need to worry about NaNs
float Pow5( float x ) {
	return x * x * x * x * x;
}


// Ref: http://jcgt.org/published/0003/02/03/paper.pdf
float SmithJointGGXVisibilityTerm( float dotNL, float dotNV, float roughness ) {
	// Approximised version
	float a = sqrt( roughness );
	float lambdaV = dotNL * ( dotNV * ( 1.0f - a ) + a );
	float lambdaL = dotNV * ( dotNL * ( 1.0f - a ) + a );
	return 0.5f * rcp( lambdaV + lambdaL );
}


float GGXTerm( float dotNH, float roughness ) {
	float a2 = roughness * roughness;
	float d = ( dotNH * a2 - dotNH ) * dotNH + 1.0f; // 2 mad
	return INV_PI * a2 / ( d * d );
}


float3 FresnelTerm( float3 F0, float cosA ) {
	float t = Pow5( 1.0f - cosA );	// ala Schlick interpoliation
	// Christian Schuler solution’s for specular micro-occlusion
	// Since we know that no real-world material has a value of f0 lower than 0.02 any 
	// values of reflectance lower than this can be assumed to be the result of pre-baked
	// occlusion and thus smoothly decreases the Fresnel reflectance contribution
	float f90 = saturate ( 50.f * dot( F0, 0.33f ) );
	return lerp( F0, f90, t );
}


float3 CalcLight( float3 N, float3 L, float3 V, float metalness, float roughness, float3 albedo )
{
	float oneMinusReflectivity = ( 1.0f - DIELECTRIC_SPEC ) * ( 1.0f - metalness );
	float3 specularColor = lerp( DIELECTRIC_SPEC, albedo, metalness );
	albedo *= oneMinusReflectivity;
	
	// https://docs.unity3d.com/Manual/StandardShaderMaterialParameterSmoothness.html
	//float roughness = PerceptualRoughnessToRoughness( perceptualRoughness );
	roughness = max(roughness, 1e-06);

	float3 H = normalize( V + L );
	float dotLH = saturate( dot( L, H ) );
	float dotNH = saturate( dot( N, H ) );
	float dotNV = saturate( dot( N, V ) );
	float dotNL = saturate( dot( N, L ) );

	// Lambert diffuse
	float diffuseTerm = dotNL / PI;

	// Specular term
	float visTerm = SmithJointGGXVisibilityTerm( dotNL, dotNV, roughness );
	float distTerm = GGXTerm( dotNH, roughness );
	float specularTerm = visTerm * distTerm; // Torrance-Sparrow model, Fresnel is applied later

	// specularTerm * dotNL can be NaN on Metal in some cases, use max() to make sure it's a sane value
	specularTerm = max( 0.0f, specularTerm * dotNL );

	return albedo * diffuseTerm + specularTerm * FresnelTerm( specularColor, dotLH );
}


float2 Hammersley( uint Index, uint NumSamples, uint2 Random )
{
	float E1 = frac( (float)Index / NumSamples + float( Random.x & 0xffff ) / (1<<16) );
	float E2 = float( reversebits(Index) ^ Random.y ) * 2.3283064365386963e-10;
	return float2( E1, E2 );
}


float3 ImportanceSampleGGX( float2 E, float Roughness, float3 N )
{
	float m = Roughness * Roughness;
	float m2 = m * m;

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


float3 ImportanceSampleGGX( float2 E, float Roughness )
{
	float m = Roughness * Roughness;
	float m2 = m * m;

	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt( (1 - E.y) / ( 1 + (m2 - 1) * E.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	float3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	return H;
}


uint3 RandVector(int3 p)
{
	uint3 v = uint3(p);
	v = v * 1664525u + 1013904223u;
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;
	return v >> 16u;
}


float3 GetEnvironmentLight( float3 N, float3 V, float metalness, float roughness, float3 albedo, uint2 random )
{
	float oneMinusReflectivity = ( 1.0f - DIELECTRIC_SPEC ) * ( 1.0f - metalness );
	float3 specularColor = lerp( DIELECTRIC_SPEC, albedo, metalness );
	
	// https://docs.unity3d.com/Manual/StandardShaderMaterialParameterSmoothness.html
	//float roughness = PerceptualRoughnessToRoughness( perceptualRoughness );
	roughness = max( roughness, 1e-06 );

	float3 SpecularLighting = 0;
	const uint NumSamples = 128;
	for( uint i = 0; i < NumSamples; i++ )
	{
		float2 Xi = Hammersley( i, NumSamples, random );
		float3 H = ImportanceSampleGGX( Xi, roughness, N );
		float3 L = 2 * dot( V, H ) * H - V;
		float NoV = saturate( dot( N, V ) );
		float NoL = saturate( dot( N, L ) );
		float NoH = saturate( dot( N, H ) );
		float VoH = saturate( dot( V, H ) );
		if( NoL > 0 )
		{
			float3 SampleColor = EnvironmentMap.SampleLevel( LinearWrapSampler , L, 0 ).rgb;
			float Vis = SmithJointGGXVisibilityTerm( NoL, NoV, roughness );
			float Fc = pow( 1 - VoH, 5 );
			float3 F = (1 - Fc) * specularColor + Fc;
			SpecularLighting += SampleColor * F * ( NoL * Vis * (4 * VoH / NoH) );
		}
	}
	return SpecularLighting / NumSamples;
}