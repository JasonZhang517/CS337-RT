#include "RTCommon.hlsli"

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
cbuffer cbInstanceIdx : register(b3)
{
    uint g_firstInstanceIdx;
};

//--------------------------------------------------------------------------------------
// Texture and buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<float3> g_vertexColors[] : register(u0);

//--------------------------------------------------------------------------------------
// Ray generation
//--------------------------------------------------------------------------------------
[shader("raygeneration")]
void raygenMain()
{
    uint vertexIdx = DispatchRaysIndex().x;
    
    float3 rayOrigin = l_eyePt;
    float3 hitObjPos = g_vertexBuffers[g_firstInstanceIdx][vertexIdx].Pos;
    float4 hitPos4 = mul(float4(hitObjPos, 1.0), g_worlds[g_firstInstanceIdx]);
    float3 hitPos = hitPos4.xyz / hitPos4.w;
    float3 rayDirection = normalize(hitPos - l_eyePt);

    float3 color = traceRadianceRay(rayOrigin, rayDirection, 0);
    
    g_vertexColors[g_firstInstanceIdx][vertexIdx] = color;
}