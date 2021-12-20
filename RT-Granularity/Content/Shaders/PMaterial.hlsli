#define NUM_MESH 2

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register(b0)
{
    float4 g_color_specexp[NUM_MESH];
    float4 g_albedo[NUM_MESH];
};

float4 getAlbedo(uint instanceIdx)
{
    // return float4(g_albedo[instanceIdx], 1, 1);
    return g_albedo[instanceIdx];
}

float3 getColor(uint instanceIdx)
{
    return g_color_specexp[instanceIdx].xyz;
}

float getSpecularExponent(uint instanceIdx)
{
    return g_color_specexp[instanceIdx].w;
}
