#define NUM_MESH 2

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
cbuffer cbMaterial : register(b0)
{
    float4 g_baseColor[NUM_MESH];
    float4 g_albedo[NUM_MESH];
};

float4 getAlbedo(uint instanceIdx)
{
    return g_albedo[instanceIdx];
}

float3 getColor(uint instanceIdx)
{
    return g_baseColor[instanceIdx].xyz;
}

float getSpecularExponent(uint instanceIdx)
{
    return g_baseColor[instanceIdx].w;
}
