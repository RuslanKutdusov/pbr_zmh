struct Vertex
{
    float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 tex : TEXTURE0;
};


struct VSOutput
{
	float4 pos : SV_Position;
};


cbuffer Params
{
	float4x4 ViewProjMatrix;
	float4x4 WorldMatrix;
};


VSOutput vs_main( Vertex input )
{
    VSOutput output;

    float4 pos = mul( float4( input.pos, 1 ), ViewProjMatrix);
    output.pos = mul( pos, WorldMatrix );

    return output;
}


float4 ps_main( VSOutput input ) : SV_Target
{
    float4 ret = float4( 1.0f, 1.0f, 1.0f, 1.0f );
	return ret;
}


DepthStencilState EnableDepthTestWrite
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
};


RasterizerState RasterState
{
	CullMode = BACK;
	MultisampleEnable = FALSE;
};


technique10 Lighting
{
    pass p0
    {
        SetVertexShader( CompileShader( vs_4_0, vs_main() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ps_main() ) );
        
        SetDepthStencilState( EnableDepthTestWrite, 0 );
        SetRasterizerState( RasterState );
    }  
}