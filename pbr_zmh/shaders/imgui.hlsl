Texture2D Texture : register( t0 );


struct VertexInput
{
	float2  pos : POSITION;
    float2  uv : UV;
    float4  color : COLOR;
};


struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : UV;
	float4 color : COLOR;
};


VSOutput vs_main( VertexInput input )
{
	VSOutput output = (VSOutput)0;

	output.pos.xy = input.pos.xy * 2 / float2( ScreenWidth, ScreenHeight ) - 1;
	output.pos.y *= -1;
	output.pos.w = 1;
	output.uv = input.uv;
	output.color = input.color;

	return output;
}


float4 ps_main( VSOutput input ) : SV_Target
{
	return Texture.Sample( LinearWrapSampler, input.uv ) * input.color; 
}