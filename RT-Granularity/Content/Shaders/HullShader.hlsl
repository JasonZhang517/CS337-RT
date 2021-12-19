struct VS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : WORLDPOS;
	float3 vNormal : NORMAL;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : WORLDPOS;
	float3 vNormal : NORMAL;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

float ComputeWeight(InputPatch<VS_CONTROL_POINT_OUTPUT, 3> inPatch, int i, int j)
{
	return dot(inPatch[j].vPosition - inPatch[i].vPosition, inPatch[i].vNormal);
}

float3 ComputeEdgePosition(InputPatch<VS_CONTROL_POINT_OUTPUT, 3> inPatch, int i, int j)
{
	return (
		(2.0f * inPatch[i].vPosition) + inPatch[j].vPosition
		- (ComputeWeight(inPatch, i, j) * inPatch[i].vNormal)
		) / 3.0f;
}

float3 ComputeEdgeNormal(InputPatch<VS_CONTROL_POINT_OUTPUT, 3> inPatch, int i, int j)
{
	float t = dot(inPatch[j].vPosition - inPatch[i].vPosition,
		inPatch[i].vNormal + inPatch[j].vNormal);
	float b = dot(inPatch[j].vPosition - inPatch[i].vPosition,
		inPatch[j].vPosition - inPatch[i].vPosition);
	float v = 2.0f * (t / b);
	return normalize(
		inPatch[i].vNormal + inPatch[j].vNormal
		-v*(inPatch[j].vPosition-inPatch[i].vPosition)
	);
}

#define NUM_CONTROL_POINTS 13

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, 3> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	Output.EdgeTessFactor[0] = 64.0f;
	Output.EdgeTessFactor[1] = 64.0f;
		Output.EdgeTessFactor[2] = 64.0f;

	Output.InsideTessFactor = 64.0f;

	return Output;
}

[domain("tri")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main( 
	InputPatch<VS_CONTROL_POINT_OUTPUT, 3> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT Output;

	Output.vPosition = float3(0.0f,0.0f,0.0f);
	Output.vNormal = float3(0.0f, 0.0f, 0.0f);

	switch (i)
	{
	case 0:

	case 1:

	case 2:
		Output.vPosition = ip[i].vPosition;
		Output.vNormal = ip[i].vNormal;
		break;
	//edge between v0 and v1
	//b(210)
	case 3:
		Output.vPosition = ComputeEdgePosition(ip, 0, 1);
		break;
	//b(120)
	case 4:
		Output.vPosition = ComputeEdgePosition(ip, 1, 0);
		break;
	//edge between v1 and v2
	//b(021)
	case 5:
		Output.vPosition = ComputeEdgePosition(ip, 1, 2);
		break;
	//b(012)
	case 6:
		Output.vPosition = ComputeEdgePosition(ip, 2, 1);
		break;
	//edge between v2 and v0
	//b(102)
	case 7:
		Output.vPosition = ComputeEdgePosition(ip, 2, 0);
		break;
	//b(201)
	case 8:
		Output.vPosition = ComputeEdgePosition(ip, 0, 2);
		break;
	//b(111)
	case 9:
		float3 E = (ComputeEdgePosition(ip, 0, 1) + ComputeEdgePosition(ip, 1, 0) + ComputeEdgePosition(ip, 2, 0) + ComputeEdgePosition(ip, 0, 2)
			+ ComputeEdgePosition(ip, 2, 1) + ComputeEdgePosition(ip, 1, 2)) / 6.0f;
		float3 V = (ip[0].vPosition+ ip[1].vPosition+ ip[2].vPosition) / 3.0f;
		Output.vPosition = E + ((E - V) / 2.0f);
		break;
	//n(110)
	case 10:
		Output.vNormal = ComputeEdgeNormal(ip, 0, 1);
		break;
	case 11:
		Output.vNormal = ComputeEdgeNormal(ip, 1, 2);
		break;
	case 12:
		Output.vNormal = ComputeEdgeNormal(ip, 2, 0);
		break;
	}
	return Output;
}
