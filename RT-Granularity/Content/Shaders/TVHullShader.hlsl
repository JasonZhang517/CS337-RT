#include "TessCommon.hlsli"

struct VSOut
{
    float3 Pos : Position;
};

#define NUM_PATCH_POINTS   3
#define NUM_CONTROL_POINTS 3

HSConstOut CalcHSPatchConstants(
    InputPatch<VSOut, NUM_PATCH_POINTS> ip,
    uint PatchID : SV_PrimitiveID)
{
    HSConstOut Output;

    Output.EdgeTessFactor[0] =
    Output.EdgeTessFactor[1] =
    Output.EdgeTessFactor[2] =
    Output.InsideTessFactor = g_tessFactor;

    return Output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSControlOut main(
    InputPatch<VSOut, NUM_PATCH_POINTS> ip,
    uint i : SV_OutputControlPointID,
    uint PatchID : SV_PrimitiveID)
{
    HSControlOut Output;

    Output.Pos = ip[i].Pos;

    return Output;
}
