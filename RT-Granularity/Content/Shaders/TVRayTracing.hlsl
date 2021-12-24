#include "RTCommon.hlsli"

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
cbuffer cbTessellation : register(b3)
{
    uint g_firstInstanceIdx;
    uint g_tessFactor;
    uint g_maxVertPerPatch;
};

//--------------------------------------------------------------------------------------
// Texture and buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<float3> g_tessColors[] : register(u0);
StructuredBuffer<float2>  g_tessDomains[] : register(t2);

//--------------------------------------------------------------------------------------
// Ray generation
//--------------------------------------------------------------------------------------
[shader("raygeneration")]
void raygenMain()
{
    uint vertIdx = DispatchRaysIndex().x;
    
    float2 dom = g_tessDomains[g_firstInstanceIdx][vertIdx];
    float u = dom.x, v = dom.y, w = 1 - (dom.x + dom.y);
    
    if (u >= 0 && v >= 0 && w >= 0)
    {
        uint patchIdx = vertIdx / g_maxVertPerPatch;
        uint baseIdx = patchIdx * 3;
        uint3 firstIndices =
        {
            g_indexBuffers[g_firstInstanceIdx][baseIdx],
            g_indexBuffers[g_firstInstanceIdx][baseIdx + 1],
            g_indexBuffers[g_firstInstanceIdx][baseIdx + 2]
        };
        float3 firstVertices[] =
        {
            g_vertexBuffers[g_firstInstanceIdx][firstIndices[0]].Pos,
            g_vertexBuffers[g_firstInstanceIdx][firstIndices[1]].Pos,
            g_vertexBuffers[g_firstInstanceIdx][firstIndices[2]].Pos
        };
        float3 hitObjPos = firstVertices[0] * u + firstVertices[1] * v + firstVertices[2] * w;
        float4 hitPos4 = mul(float4(hitObjPos, 1.0), g_worlds[g_firstInstanceIdx]);
        float3 hitPos = hitPos4.xyz / hitPos4.w;

        float3 color = traceRadianceRay(l_eyePt, normalize(hitPos - l_eyePt), 0);
    
        g_tessColors[g_firstInstanceIdx][vertIdx] = color;
    }
    else
    {
        g_tessColors[g_firstInstanceIdx][vertIdx] = float3(0, 0, 0);
    }
}
