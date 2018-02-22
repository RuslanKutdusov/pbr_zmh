#include "global.h"
#include "lighting.h"

cbuffer Constants : register( b1 )
{
	uint MipIndex;
	uint MipsNumber;
	uint2 padding0;
};

RWTexture2DArray< float4 > PrefilteredEnvMapUAV : register( u0 );

[numthreads(32, 32, 1)]
void cs_main( uint2 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID )
{
	uint faceIndex = groupId.z;
	uint width, height, arraySize;
	PrefilteredEnvMapUAV.GetDimensions( width, height, arraySize );
	if( id.x >= width || id.y >= height )
		return;
	
	float3 vec = float3( 0.0, id.y, id.x ) / ( float )width;
	// shift to the center of pixel
	vec.yz += 0.5 / ( float )width;
	//      +Y
	//       ^ +X
	//       |/
	// +Z <--+
	//
	vec.yz = 1.0 - vec.yz;
	// [0.0, 1.0] -> [ -1.0, 1.0 ]
	vec.yz = vec.yz * 2.0 - 1.0;
	vec.x = 1.0;
	vec = normalize( vec );
	
	float3x3 rotation[ 6 ] = {
		float3x3( float3(  1,  0,  0 ), float3(  0, 1,  0 ), float3(  0, 0,  1 ) ), // +X
		float3x3( float3( -1,  0,  0 ), float3(  0, 1,  0 ), float3(  0, 0, -1 ) ), // -X 
		float3x3( float3(  0,  1,  0 ), float3(  0, 0, -1 ), float3( -1, 0,  0 ) ), // +Y
		float3x3( float3(  0, -1,  0 ), float3(  0, 0,  1 ), float3( -1, 0,  0 ) ), // -Y
		float3x3( float3(  0,  0,  1 ), float3(  0, 1,  0 ), float3( -1, 0,  0 ) ), // +Z
		float3x3( float3(  0,  0, -1 ), float3(  0, 1,  0 ), float3(  1, 0,  0 ) ), // -Z
	};
	
	vec = mul( vec, rotation[ faceIndex ] );
	
	uint2 random = RandVector_v2( id.xy );
#ifdef SPECULAR
	float roughness = ( float )MipIndex / ( float )MipsNumber;
	roughness *= roughness;
	float3 L = PrefilterSpecularEnvMap( roughness, vec, vec, random );
#else
	float3 L = PrefilterDiffuseEnvMap( vec, random );
#endif
	
	PrefilteredEnvMapUAV[ uint3( id.xy, faceIndex ) ] = float4( L.xyz, 0 );
}