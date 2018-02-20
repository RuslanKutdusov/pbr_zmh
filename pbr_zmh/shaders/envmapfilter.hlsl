#include "global.h"
#include "lighting.h"

cbuffer Constants : register( b1 )
{
	uint FaceIndex;
	float TargetRoughness;
	float2 padding0;
};

RWTexture2DArray< float4 > PrefilterEnvMapUAV : register( u0 );

[numthreads(32, 32, 1)]
void cs_main( uint2 id : SV_DispatchThreadID, uint3 groupId : SV_GroupID )
{
	//uint faceIndex = groupId.z;
	float width, height, arraySize;
	// returns dimensions of mip
	PrefilterEnvMapUAV.GetDimensions( width, height, arraySize );
	float3 vec = float3( 0.0, id.y, id.x ) / width;
	// shift to the center of pixel
	vec.yz += 0.5 / width;
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
	
	vec = mul( vec, rotation[ FaceIndex ] );
	
	uint2 random = RandVector_v2( id.xy );
	float3 L = PrefilterEnvMap( TargetRoughness, vec, random );
	
	//float3 L = EnvironmentMap.SampleLevel( LinearWrapSampler, vec, 0 ).rgb;
	PrefilterEnvMapUAV[ uint3( id.xy, FaceIndex ) ] = float4( L.xyz, height );
}