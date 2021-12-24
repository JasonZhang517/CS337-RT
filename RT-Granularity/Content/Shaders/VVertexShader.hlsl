#include "VCommon.hlsli"

cbuffer cbInstanceIdx : register(b0)
{
    uint g_instanceIdx;
};

StructuredBuffer<float3> g_vertexColors[] : register(t0);

VSOut main(VSIn input)
{
    VSOut output;

    output.Pos = input.Pos;
    output.Color = g_vertexColors[g_instanceIdx][input.vId];

    return output;
}