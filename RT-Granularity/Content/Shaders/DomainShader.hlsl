struct HSOut
{
	float3 Pos : WORLDPOS;
	float3 Norm : NORMAL;
};

struct DSOut
{
	float4 Pos : SV_POSITION;
	float3 Norm : NORMAL;
};

struct HSConstOut
{
	float EdgeTessFactor[3] : SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 13

[domain("tri")]
DSOut main(
	HSConstOut input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HSOut, NUM_CONTROL_POINTS> patch)
{
	DSOut Output;

	float u = domain.x;
	float v = domain.y;
	float w = domain.z;

	float3 vWorldPos = 
		  (patch[0].Pos * pow(w, 3)) 
	    + (patch[1].Pos * pow(u, 3)) 
	    + (patch[2].Pos * pow(v, 3))
		+ (patch[3].Pos * 3.0f * pow(w, 2) * u)
		+ (patch[4].Pos * 3.0f * w * pow(u, 2))
		+ (patch[8].Pos * 3.0f * pow(w, 2) * v)
		+ (patch[5].Pos * 3.0f * pow(u, 2) * v)
		+ (patch[7].Pos * 3.0f * w * pow(v, 2))
		+ (patch[6].Pos * 3.0f * u * pow(v, 2))
		+ (patch[9].Pos * 6.0f * w * u * v);
	Output.Pos = float4(vWorldPos, 1);

	float3 vWorldNorm =
		  (pow(w, 2) * patch[0].Norm)
		+ (pow(u, 2) * patch[1].Norm)
		+ (pow(v, 2) * patch[2].Norm)
		+ (w * u * patch[10].Norm)
		+ (u * v * patch[11].Norm)
		+ (w * v * patch[12].Norm);
	// float3 vWorldNorm = w * patch[0].Norm + u * patch[1].Norm + v * patch[2].Norm;
	Output.Norm = normalize(vWorldNorm);

	return Output;
}