//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct VSIn
{
	float3	Pos	: POSITION;
	float3	Nrm	: NORMAL;
};

struct VSOut
{
	float4	Pos		: POSITION;
	float4	CSPos	: POSCURRENT;
	float4	TSPos 	: POSHISTORY;
	float3	Norm	: NORMAL;
	float2	UV		: TEXCOORD;
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

float3 toXYZ(float4 pos)
{
	return pos.xyz / pos.w;
}

//--------------------------------------------------------------------------------------
// Base geometry pass
//--------------------------------------------------------------------------------------
VSOut main(VSIn input)
{
	VSOut output;

/*
	const float4 pos = { input.Pos, 1.0 };
	const float4 tpos = mul(pos, g_worldViewProjPrev);
	float4 cpos = mul(pos, g_worldViewProj);
	output.TSPos = toXYZ(tpos);
	output.CSPos = toXYZ(cpos);
	output.Norm = mul(input.Nrm, g_worldIT);
	output.UV = cpos.xz * 0.5 + 0.5;

	cpos.xy += g_projBias * cpos.w;
	output.Pos = toXYZ(cpos);
*/
	const float4 pos = { input.Pos, 1.0 };
	output.Pos = mul(pos, g_worldViewProj);
	output.TSPos = mul(pos, g_worldViewProjPrev);
	output.CSPos = output.Pos;

	output.Pos.xy += g_projBias * output.Pos.w;
	output.Norm = mul(input.Nrm, g_worldIT);
	output.UV = pos.xz * 0.5 + 0.5;
	
	return output;
}
