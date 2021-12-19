struct VSOut
{
	float3 Pos : WORLDPOS;
	float3 Norm : NORMAL;
};

struct HSControlOut
{
	float3 Pos : WORLDPOS;
	float3 Norm : NORMAL;
};

struct HSConstOut
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
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
	Output.InsideTessFactor = 1;

	return Output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HSControlOut main( 
	InputPatch<VSOut, NUM_CONTROL_POINTS> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HSControlOut Output;

	Output.Pos = ip[i].Pos;
	Output.Norm = ip[i].Norm;

	return Output;
}
