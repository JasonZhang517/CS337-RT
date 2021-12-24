#include "TVTessCommon.hlsli"

struct PSIn
{
    float4 Pos : SV_Position;
    float3 Color : Color;
};

cbuffer cbGraphics : register(b1)
{
    matrix   g_worldViewProj;
    float3x3 g_worldIT;
    float2   g_projBias;
};

StructuredBuffer<float3> g_tessColors[] : register(t0);

[domain("tri")]
PSIn main(
    HSConstOut input,
    float3 domain : SV_DomainLocation,
    const OutputPatch<HSControlOut, 3> patch,
    uint patchID : SV_PrimitiveID)
{
    PSIn output;
    
    const uint baseIdx = g_maxVertPerPatch * patchID;
    
    float3 pos = domain.x * patch[0].Pos + domain.y * patch[1].Pos + domain.z * patch[2].Pos;
    output.Pos = mul(float4(pos, 1), g_worldViewProj);
    output.Pos.xy += g_projBias * output.Pos.w;
    output.Color = g_tessColors[g_instanceIdx][baseIdx + domainHash(domain.xy, g_tessFactor, g_maxVertPerPatch)];

    return output;
}
