uint3 RandVector_v1(int3 p)
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


uint hash(uint x, uint y)
{
    const uint M = 1664525u, C = 1013904223u;
    uint seed = (x * M + y + C) * M;
    seed ^= (seed >> 11u);
    seed ^= (seed << 7u) & 0x9d2c5680u;
    seed ^= (seed << 15u) & 0xefc60000u;
    seed ^= (seed >> 18u);
    return seed;
}


uint2 RandVector_v2(uint2 p)
{
	uint seed1 = hash(uint(p.x), uint(p.y));
    uint seed2 = hash(seed1, 1000);
    return uint2( seed1, seed2 );
}


float2 Hammersley_v1( uint Index, uint NumSamples, uint2 Random )
{
	float E1 = frac( (float)Index / NumSamples + float( Random.x & 0xffff ) / (1<<16) );
	float E2 = float( reversebits(Index) ^ Random.y ) * 2.3283064365386963e-10;
	return float2( E1, E2 );
}


float2 Hammersley_v2( uint i, uint N ) 
{
    float ri = reversebits(i) * 2.3283064365386963e-10f;
    return float2(float(i) / float(N), ri);
}


float HammersleySample(uint bits, uint seed)
{
    bits = ( bits << 16u) | ( bits >> 16u);
    bits = ((bits & 0x00ff00ffu) << 8u) | ((bits & 0xff00ff00u) >> 8u);
    bits = ((bits & 0x0f0f0f0fu) << 4u) | ((bits & 0xf0f0f0f0u) >> 4u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xccccccccu) >> 2u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xaaaaaaaau) >> 1u);
    bits ^= seed;
    return float(bits) * 2.3283064365386963e-10;
}


float2 Hammersley_v3( uint Index, uint NumSamples, uint2 Random )
{
    float E1 = frac( (float)Index / NumSamples + float( Random.x ) * 2.3283064365386963e-10 );
    float E2 = frac( HammersleySample( Index, Random.y ) );
	return float2( E1, E2 );
}