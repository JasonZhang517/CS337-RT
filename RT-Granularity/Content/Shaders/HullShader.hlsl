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
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};

float ComputeWeight(InputPatch<VSOut, 3> inPatch, int i, int j)
{
    return dot(inPatch[j].Pos - inPatch[i].Pos, inPatch[i].Norm);
}

float3 ComputeEdgePosition(InputPatch<VSOut, 3> inPatch, int i, int j)
{
    return (
        (2.0f * inPatch[i].Pos) + inPatch[j].Pos
        - (ComputeWeight(inPatch, i, j) * inPatch[i].Norm)
        ) / 3.0f;
}

float3 ComputeEdgeNormal(InputPatch<VSOut, 3> inPatch, int i, int j)
{
    float t = dot(inPatch[j].Pos - inPatch[i].Pos,
        inPatch[i].Norm + inPatch[j].Norm);
    float b = dot(inPatch[j].Pos - inPatch[i].Pos,
        inPatch[j].Pos - inPatch[i].Pos);
    float v = 2.0f * (t / b);
    return normalize(
        inPatch[i].Norm + inPatch[j].Norm
        - v * (inPatch[j].Pos - inPatch[i].Pos)
    );
}

#define NUM_CONTROL_POINTS 13

HSConstOut CalcHSPatchConstants(
    InputPatch<VSOut, 3> ip,
    uint PatchID : SV_PrimitiveID)
{
    HSConstOut Output;

    Output.EdgeTessFactor[0] = 4.0f;
    Output.EdgeTessFactor[1] = 4.0f;
    Output.EdgeTessFactor[2] = 4.0f;

    Output.InsideTessFactor = 4.0f;

    return Output;
}

[domain("tri")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSControlOut main( 
    InputPatch<VSOut, 3> ip, 
    uint i : SV_OutputControlPointID,
    uint PatchID : SV_PrimitiveID )
{
    HSControlOut Output;

    Output.Pos = float3(0.0f, 0.0f, 0.0f);
    Output.Norm = float3(0.0f, 0.0f, 0.0f);

    switch (i)
    {
        case 0: case 1: case 2: {
            Output.Pos = ip[i].Pos;
            Output.Norm = ip[i].Norm;
        } break;
        //edge between v0 and v1
        //b(210)
        case 3: {
            Output.Pos = ComputeEdgePosition(ip, 0, 1);
        } break;
        //b(120)
        case 4: {
            Output.Pos = ComputeEdgePosition(ip, 1, 0);
        } break;
        //edge between v1 and v2
        //b(021)
        case 5: {
            Output.Pos = ComputeEdgePosition(ip, 1, 2);
        } break;
        //b(012)
        case 6: {
            Output.Pos = ComputeEdgePosition(ip, 2, 1);
        } break;
        //edge between v2 and v0
        //b(102)
        case 7: {
            Output.Pos = ComputeEdgePosition(ip, 2, 0);
        } break;
        //b(201)
        case 8: {
            Output.Pos = ComputeEdgePosition(ip, 0, 2);
        } break;
        //b(111)
        case 9: {
            float3 E = 
                (ComputeEdgePosition(ip, 0, 1) 
               + ComputeEdgePosition(ip, 1, 0)
               + ComputeEdgePosition(ip, 2, 0)
               + ComputeEdgePosition(ip, 0, 2)
               + ComputeEdgePosition(ip, 2, 1)
               + ComputeEdgePosition(ip, 1, 2)) / 6.0f;
            float3 V = (ip[0].Pos + ip[1].Pos + ip[2].Pos) / 3.0f;
            Output.Pos = E + ((E - V) / 2.0f);
        } break;
        //n(110)
        case 10: {
            Output.Norm = ComputeEdgeNormal(ip, 0, 1);
        } break;
        case 11: {
            Output.Norm = ComputeEdgeNormal(ip, 1, 2);
        } break;
        case 12: {
            Output.Norm = ComputeEdgeNormal(ip, 2, 0);
        } break;
    }
    return Output;
}
