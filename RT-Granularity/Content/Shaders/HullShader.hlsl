struct VSOut
{
	float4	Pos		: POSITION;
	float4	CSPos	: POSCURRENT;
	float4	TSPos 	: POSHISTORY;
	float3	Norm	: NORMAL;
	float2	UV		: TEXCOORD;
};

struct HSOut
{
	float4	Pos		: POSITION;
	float4	CSPos	: POSCURRENT;
	float4	TSPos 	: POSHISTORY;
	float3	Norm	: NORMAL;
	float2	UV		: TEXCOORD;
};

/*
struct VS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : WORLDPOS;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : WORLDPOS;
};
*/

struct HSConstOut
{
	float EdgeTessFactor[3]	: SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 3

HSConstOut CalcHSPatchConstants(
	InputPatch<VSOut, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HSConstOut Output;

	Output.EdgeTessFactor[0] = 
		Output.EdgeTessFactor[1] = 
		Output.EdgeTessFactor[2] = 
		Output.InsideTessFactor = 3;

	return Output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSOut main(
	InputPatch<VSOut, NUM_CONTROL_POINTS> ip,
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HSOut Output;

	Output.Pos = ip[i].Pos;
	Output.CSPos = ip[i].CSPos;
	Output.TSPos = ip[i].TSPos;
	Output.Norm = ip[i].Norm;
	Output.UV = ip[i].UV;

	return Output;
}
