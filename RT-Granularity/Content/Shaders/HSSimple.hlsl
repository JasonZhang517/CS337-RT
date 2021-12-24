struct Vertex
{
    float3 Pos  : Position;
    float3 Norm : Normal;
};

struct HSControlOut
{
    float3 Pos  : Position;
};

struct HSConstOut
{
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor  : SV_InsideTessFactor;
    uint  PatchID;
};

cbuffer cbTessellation : register(b0)
{
    uint g_instanceIdx;
    uint g_tessFactor;
};

#define NUM_PATCH_POINTS   3
#define NUM_CONTROL_POINTS 3

HSConstOut CalcHSPatchConstants(
    InputPatch<Vertex, NUM_PATCH_POINTS> ip,
    uint PatchID : SV_PrimitiveID)
{
    HSConstOut Output;

    Output.EdgeTessFactor[0] = 
    Output.EdgeTessFactor[1] = 
    Output.EdgeTessFactor[2] = 
    Output.InsideTessFactor = g_tessFactor;
    
    Output.PatchID = PatchID;

    return Output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSControlOut main(
    InputPatch<Vertex, NUM_PATCH_POINTS> ip,
    uint i : SV_OutputControlPointID,
    uint PatchID : SV_PrimitiveID)
{
    HSControlOut Output;

    Output.Pos = ip[i].Pos;

    return Output;
}
