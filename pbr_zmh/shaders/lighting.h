static const float DIELECTRIC_SPEC = 0.04f;
static const float PI = 3.14159265359f;
static const float INV_PI = 0.31830988618f;


//
float PerceptualRoughnessToRoughness( float perceptualRoughness ) {
	return perceptualRoughness * perceptualRoughness;
}


// Pow5 uses the same amount of instructions as generic pow(), but has 2 advantages:
// 1) better instruction pipelining
// 2) no need to worry about NaNs
float Pow5( float x ) {
	return x * x * x * x * x;
}


// Ref: http://jcgt.org/published/0003/02/03/paper.pdf
float SmithJointGGXVisibilityTerm( float dotNL, float dotNV, float roughness ) {
	// Approximised version
	float a = roughness;
	float lambdaV = dotNL * ( dotNV * ( 1.0f - a ) + a );
	float lambdaL = dotNV * ( dotNL * ( 1.0f - a ) + a );
	return 0.5f / ( lambdaV + lambdaL );
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


float3 CalcLight( float3 N, float3 L, float3 V, float metalness, float perceptualRoughness, float3 albedo )
{
	float oneMinusReflectivity = ( 1.0f - DIELECTRIC_SPEC ) * ( 1.0f - metalness );
	float3 specularColor = lerp( DIELECTRIC_SPEC, albedo, metalness );
	albedo *= oneMinusReflectivity;
	
	// https://docs.unity3d.com/Manual/StandardShaderMaterialParameterSmoothness.html
	float roughness = PerceptualRoughnessToRoughness( perceptualRoughness );
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