struct DSOut
{
    float4 Pos  : SV_Position;
    float3 Norm : Normal;
};

struct HSControlOut
{
    float3 Pos  : WorldPos;
    float3 Norm : Normal;
};

struct HSConstOut
{
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor  : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DSOut main(
    HSConstOut input,
    float3 domain : SV_DomainLocation,
    const OutputPatch<HSControlOut, NUM_CONTROL_POINTS> patch)
{
    DSOut Output;

    Output.Pos = 
        float4(patch[0].Pos * domain.x + patch[1].Pos * domain.y + patch[2].Pos * domain.z, 1);
    Output.Norm = 
        patch[0].Norm * domain.x + patch[1].Norm * domain.y + patch[2].Norm * domain.z;

    return Output;
}
