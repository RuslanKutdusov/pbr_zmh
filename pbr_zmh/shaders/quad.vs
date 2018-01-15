float4 vs_main( uint vertexId : SV_VertexID ) : SV_Position
{
	float4 ndc[ 6 ] = {
		float4( -1.0f, -1.0f, 0.0f, 1.0f ),
		float4( -1.0f,  1.0f, 0.0f, 1.0f ),
		float4(  1.0f,  1.0f, 0.0f, 1.0f ),
		float4( -1.0f, -1.0f, 0.0f, 1.0f ),
		float4(  1.0f,  1.0f, 0.0f, 1.0f ),
		float4(  1.0f, -1.0f, 0.0f, 1.0f ),
	};
	
	return ndc[ vertexId ];
}
