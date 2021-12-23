struct VSOut
{
    float3 Pos  : WorldPos;
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
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSControlOut main( 
    InputPatch<VSOut, 3> ip, 
    uint i : SV_OutputControlPointID,
    uint PatchID : SV_PrimitiveID)
{
    HSControlOut Output;

    Output.Pos = ip[i].Pos;
    Output.Norm = ip[i].Norm;

    return Output;
}
