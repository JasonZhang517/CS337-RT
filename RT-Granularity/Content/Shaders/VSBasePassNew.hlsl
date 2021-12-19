//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct VSIn
{
    float3	Pos		: POSITION;
    float3	Norm	: NORMAL;
};

struct VSOut
{
    float3	Pos		: WORLDPOS;
    float3	Norm	: NORMAL;
};

float3 toXYZ(float4 hpos)
{
    return hpos.xyz / hpos.w;
}

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerObject
{
    matrix	g_worldViewProj;
    matrix	g_worldViewProjPrev;
    float3x3 g_worldIT;
    float2	g_projBias;
};

//--------------------------------------------------------------------------------------
// Base geometry pass
//--------------------------------------------------------------------------------------
VSOut main(VSIn input)
{
    VSOut output;

    float4 pos = { input.Pos, 1.0 };
    pos = mul(pos, g_worldViewProj);
    pos.xy += g_projBias * pos.w;

    output.Pos = toXYZ(pos);
    output.Norm = mul(input.Norm, g_worldIT);

    return output;
}
