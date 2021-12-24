#include "TessCommon.hlsli"

cbuffer cbGraphics : register(b1)
{
    matrix g_worldViewProj;
    float3x3 g_worldIT;
    float2 g_projBias;
};

[domain("tri")]
float4 main(
    HSConstOut input,
    float3 domain : SV_DomainLocation,
    const OutputPatch<HSControlOut, 3> patch) : SV_Position
{
    float3 pos3 = domain.x * patch[0].Pos + domain.y * patch[1].Pos + domain.z * patch[2].Pos;
    float4 pos = mul(float4(pos3, 1), g_worldViewProj);
    pos.xy += g_projBias * pos.w;
    return pos;
}
