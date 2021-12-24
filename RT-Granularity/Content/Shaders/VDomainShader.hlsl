#include "VCommon.hlsli"

cbuffer cbGraphics : register(b0)
{
    matrix   g_worldViewProj;
    float3x3 g_worldIT;
    float2   g_projBias;
};

[domain("tri")]
PSIn main(
    HSConstOut input,
    float3 domain : SV_DomainLocation,
    const OutputPatch<HSControlOut, 3> patch,
    uint patchID : SV_PrimitiveID)
{
    PSIn output;;
    
    float3 pos = 
        domain.x * patch[0].Pos + domain.y * patch[1].Pos + domain.z * patch[2].Pos;
    output.Pos = mul(float4(pos, 1), g_worldViewProj);
    output.Pos.xy += g_projBias * output.Pos.w;
    output.Color = 
        domain.x * patch[0].Color + domain.y * patch[1].Color + domain.z * patch[2].Color;

    return output;
}
