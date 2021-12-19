//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Material.hlsli"

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct PSIn
{
	float4	Pos		: SV_POSITION;
	float3	Norm	: NORMAL;
	float2	UV		: TEXCOORD;
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
	uint g_instanceIdx;
};

//--------------------------------------------------------------------------------------
// Base geometry-buffer pass
//--------------------------------------------------------------------------------------
PSOut main(PSIn input)
{
	PSOut output;

	output.BaseColor = getBaseColor(g_instanceIdx, input.UV);
	output.Normal = min16float4(normalize(input.Norm) * 0.5 + 0.5, (g_instanceIdx + 1) / 2.0);
	output.RoughMetal = getRoughMetal(g_instanceIdx, input.UV);
	output.Velocity = min16float2(1, 1);

	return output;
}
