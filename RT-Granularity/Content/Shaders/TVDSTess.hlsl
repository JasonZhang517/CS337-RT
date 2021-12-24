#include "TessCommon.hlsli"

RWStructuredBuffer<float2> g_tessDomains[] : register(u0);

[domain("tri")]
void main(
    HSConstOut input,
    float3 domain : SV_DomainLocation,
    const OutputPatch<HSControlOut, 3> patch,
    uint patchID : SV_PrimitiveID)
{
    const uint baseIdx = g_maxVertPerPatch * patchID;
    
    g_tessDomains[g_instanceIdx][baseIdx + domainHash(domain.xy, g_tessFactor, g_maxVertPerPatch)] = domain.xy;
}
