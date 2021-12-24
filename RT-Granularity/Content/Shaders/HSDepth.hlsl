#include "TessDepthCommon.hlsli"

cbuffer cbTessellation : register(b0)
{
    uint g_tessFactor;
};

HSConstOut CalcHSPatchConstants(
    InputPatch<VSOut, 3> ip,
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
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HSControlOut main(
    InputPatch<VSOut, 3> ip,
    uint i : SV_OutputControlPointID,
    uint PatchID : SV_PrimitiveID)
{
    HSControlOut Output = { ip[i].Pos };
    return Output;
}
