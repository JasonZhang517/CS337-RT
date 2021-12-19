struct DS_OUTPUT
{
	float3 vPosition : WORLDPOS;
	float3 vNormal  : NORMAL;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : WORLDPOS;
	float3 vNormal  : NORMAL;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]   : SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 13

[domain("tri")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT Output;

	float u = domain.x;
	float v = domain.y;
	float w = domain.z;

	float3 vWorldPos = (patch[0].vPosition * pow(w, 3)) + (patch[1].vPosition * pow(u, 3)) + (patch[2].vPosition * pow(v, 3))
		+ (patch[3].vPosition * 3.0f * pow(w, 2) * u)
		+ (patch[4].vPosition * 3.0f * w * pow(u, 2))
		+ (patch[8].vPosition * 3.0f * pow(w, 2) * v)
		+ (patch[5].vPosition * 3.0f * pow(u, 2) * v)
		+ (patch[7].vPosition * 3.0f * w * pow(v, 2))
		+ (patch[6].vPosition * 3.0f * u * pow(v, 2))
		+ (patch[9].vPosition * 6.0f * w * u * v);
	Output.vPosition = vWorldPos;

	float3 vWorldNorm = (pow(w, 2) * patch[0].vNormal) + (pow(u, 2) * patch[1].vNormal) + (pow(v, 2) * patch[2].vNormal)
		+ (w * u * patch[10].vNormal) + (u * v * patch[11].vNormal) + (w * v * patch[12].vNormal);
	Output.vNormal = normalize(vWorldNorm);

	return Output;
}