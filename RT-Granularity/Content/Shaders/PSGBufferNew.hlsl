//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "MaterialNew.hlsli"

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct PSIn
{
    float4	Pos		: SV_POSITION;
    float3	Norm	: NORMAL;
};

struct PSOut
{
    min16float4 BaseColor	: SV_TARGET0;
    min16float4 Normal		: SV_TARGET1;
    min16float2 RoughMetal	: SV_TARGET2;
    min16float2 Velocity	: SV_TARGET3;
};

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

cbuffer cbPerObjectInstance
{
    uint g_instanceIdx;
};

//--------------------------------------------------------------------------------------
// Base geometry-buffer pass
//--------------------------------------------------------------------------------------
PSOut main(PSIn input)
{
    
    PSOut output;

    output.BaseColor = getBaseColor(g_instanceIdx);
    output.Normal = min16float4(normalize(input.Norm) * 0.5 + 0.5, (g_instanceIdx + 1) / 2.0);
    output.RoughMetal = getRoughMetal(g_instanceIdx);
    output.Velocity = min16float2(1, 1);

    return output;
}
