struct VSIn
{
    float3 Pos  : Position;
    float3 Norm : Normal;
    uint   vId  : SV_VertexID;
};

struct VSOut
{
    float4 Pos : SV_Position;
    float3 Color : Color;
};

cbuffer cbGraphics : register(b0)
{
    matrix   g_worldViewProj;
    float3x3 g_worldIT;
    float2   g_projBias;
};

cbuffer cbInstanceIdx : register(b1)
{
    uint g_instanceIdx;
};

StructuredBuffer<float3> g_vertexColors[] : register(t0);

VSOut main(VSIn input)
{
    VSOut output;

    output.Pos = mul(float4(input.Pos, 1.0), g_worldViewProj);
    output.Pos.xy += g_projBias * output.Pos.w;
    output.Color = g_vertexColors[g_instanceIdx][input.vId];

    return output;
}