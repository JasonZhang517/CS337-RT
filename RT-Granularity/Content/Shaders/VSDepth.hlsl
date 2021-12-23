//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Struct
//--------------------------------------------------------------------------------------
struct VSIn
{
    float3 Pos : POSITION;
    float3 Nrm : NORMAL;
};

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register(b0)
{
    matrix   g_worldViewProj;
    float3x3 g_worldIT;
    float2   g_projBias;
};

//--------------------------------------------------------------------------------------
// Base geometry pass
//--------------------------------------------------------------------------------------
float4 main(VSIn input) : SV_POSITION
{
    float4 pos = mul(float4(input.Pos, 1.0), g_worldViewProj);
    pos.xy += g_projBias * pos.w;

    return pos;
}
