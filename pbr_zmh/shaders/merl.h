/*
Copyright Disney Enterprises, Inc. All rights reserved.

This license governs use of the accompanying software. If you use the software, you
accept this license. If you do not accept the license, do not use the software.

1. Definitions
The terms "reproduce," "reproduction," "derivative works," and "distribution" have
the same meaning here as under U.S. copyright law. A "contribution" is the original
software, or any additions or changes to the software. A "contributor" is any person
that distributes its contribution under this license. "Licensed patents" are a
contributor's patent claims that read directly on its contribution.

2. Grant of Rights
(A) Copyright Grant- Subject to the terms of this license, including the license
conditions and limitations in section 3, each contributor grants you a non-exclusive,
worldwide, royalty-free copyright license to reproduce its contribution, prepare
derivative works of its contribution, and distribute its contribution or any derivative
works that you create.
(B) Patent Grant- Subject to the terms of this license, including the license
conditions and limitations in section 3, each contributor grants you a non-exclusive,
worldwide, royalty-free license under its licensed patents to make, have made,
use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the
software or derivative works of the contribution in the software.

3. Conditions and Limitations
(A) No Trademark License- This license does not grant you rights to use any
contributors' name, logo, or trademarks.
(B) If you bring a patent claim against any contributor over patents that you claim
are infringed by the software, your patent license from such contributor to the
software ends automatically.
(C) If you distribute any portion of the software, you must retain all copyright,
patent, trademark, and attribution notices that are present in the software.
(D) If you distribute any portion of the software in source code form, you may do
so only under this license by including a complete copy of this license with your
distribution. If you distribute any portion of the software in compiled or object code
form, you may only do so under a license that complies with this license.
(E) The software is licensed "as-is." You bear the risk of using it. The contributors
give no express warranties, guarantees or conditions. You may have additional
consumer rights under your local laws which this license cannot change.
To the extent permitted under your local laws, the contributors exclude the
implied warranties of merchantability, fitness for a particular purpose and non-
infringement.
*/

static const int BRDF_SAMPLING_RES_THETA_H = 90;
static const int BRDF_SAMPLING_RES_THETA_D = 90;
static const int BRDF_SAMPLING_RES_PHI_D   = 360;
static const float M_PI = 3.1415926535897932384626433832795;
static const float RED_SCALE = (1.0/1500.0);
static const float GREEN_SCALE = (1.15/1500.0);
static const float BLUE_SCALE = (1.66/1500.0);


// Lookup phi_diff index
int phi_diff_index(float phi_diff)
{
    // Because of reciprocity, the BRDF is unchanged under
    // phi_diff -> phi_diff + M_PI
    if (phi_diff < 0.0)
        phi_diff += M_PI;
    
    // In: phi_diff in [0 .. pi]
    // Out: tmp in [0 .. 179]
    return clamp(int(phi_diff * (1.0/M_PI * (BRDF_SAMPLING_RES_PHI_D / 2))), 0, BRDF_SAMPLING_RES_PHI_D / 2 - 1);
}


// Lookup theta_half index
// This is a non-linear mapping!
// In:  [0 .. pi/2]
// Out: [0 .. 89]
int theta_half_index(float theta_half)
{
    if (theta_half <= 0.0)
        return 0;

    return clamp(int(sqrt(theta_half * (2.0/M_PI)) * BRDF_SAMPLING_RES_THETA_H), 0, BRDF_SAMPLING_RES_THETA_H-1);
}


// Lookup theta_diff index
// In:  [0 .. pi/2]
// Out: [0 .. 89]
int theta_diff_index(float theta_diff)
{
    return clamp(int(theta_diff * (2.0/M_PI * BRDF_SAMPLING_RES_THETA_D)), 0, BRDF_SAMPLING_RES_THETA_D - 1);
}


float3 EvaluteMerlBRDF( Buffer<float> brdf, float3 toLight, float3 toViewer, float3 normal, float3 tangent, float3 bitangent )
{
    float3 H = normalize( toLight + toViewer );
    float theta_H = acos( saturate( dot( normal, H ) ) );
    float theta_diff = acos( saturate( dot( H, toLight ) ) );
    float phi_diff = 0;

    if( theta_diff < 1e-3 ) 
    {
        // phi_diff indeterminate, use phi_half instead
        phi_diff = atan2( clamp( -dot( toLight, bitangent ), -1, 1 ),
        				clamp( dot( toLight, tangent ), -1, 1 ) );
    }
    else if( theta_H > 1e-3 )
    {
        // use Gram-Schmidt orthonormalization to find diff basis vectors
        float3 u = -normalize( normal - dot( normal, H ) * H );
        float3 v = cross( H, u );
        phi_diff = atan2( clamp( dot( toLight, v ), -1, 1 ), 
        				clamp( dot( toLight, u ), -1, 1 ) );
    } 
    else
    { 
    	theta_H = 0;
    }

    // Find index.
    // Note that phi_half is ignored, since isotropic BRDFs are assumed
    int ind = phi_diff_index( phi_diff ) +
        theta_diff_index( theta_diff ) * BRDF_SAMPLING_RES_PHI_D / 2 +
        theta_half_index( theta_H ) * BRDF_SAMPLING_RES_PHI_D / 2 *
        BRDF_SAMPLING_RES_THETA_D;

    int redIndex = ind;
    int greenIndex = ind + BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * BRDF_SAMPLING_RES_PHI_D / 2;
    int blueIndex = ind + BRDF_SAMPLING_RES_THETA_H * BRDF_SAMPLING_RES_THETA_D * BRDF_SAMPLING_RES_PHI_D;

    float NoL = saturate( dot( toLight, normal ) );
    return float3(
				brdf[ redIndex ].r * RED_SCALE,
                brdf[ greenIndex ].r * GREEN_SCALE,
                brdf[ blueIndex ].r * BLUE_SCALE
                ) * NoL;
}
