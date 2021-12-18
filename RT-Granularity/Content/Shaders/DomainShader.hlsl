/*
struct DS_OUTPUT
{
	float4 vPosition  : SV_POSITION;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : WORLDPOS; 
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};
*/
struct DSOut
{
	float4	Pos		: SV_POSITION;
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

struct HSConstOut
{
	float EdgeTessFactor[3]	: SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DSOut main(
	HSConstOut input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HSOut, NUM_CONTROL_POINTS> patch)
{
	DSOut Output;

	Output.Pos = patch[0].Pos * domain.x + patch[1].Pos * domain.y + patch[2].Pos * domain.z;
	Output.CSPos = patch[0].CSPos * domain.x + patch[1].CSPos * domain.y + patch[2].CSPos * domain.z;
	Output.TSPos = patch[0].TSPos * domain.x + patch[1].TSPos * domain.y + patch[2].TSPos * domain.z;
	Output.Norm = patch[0].Norm * domain.x + patch[1].Norm * domain.y + patch[2].Norm * domain.z;
	Output.UV = patch[0].UV * domain.x + patch[1].UV * domain.y + patch[2].UV * domain.z;

	return Output;
}
