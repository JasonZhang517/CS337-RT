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
    uint patchIdx = vertIdx / g_maxVertPerPatch;
    
    float2 dom = g_tessDomains[g_firstInstanceIdx][vertIdx];
    float u = dom.x, v = dom.y, w = 1 - (dom.x + dom.y);
    
    if (u >= 0 && v >= 0 && w >= 0)
    {
        float3 rayOrigin = l_eyePt;
        uint3 firstIndices =
        {
            g_indexBuffers[NonUniformResourceIndex(g_firstInstanceIdx)][patchIdx],
            g_indexBuffers[NonUniformResourceIndex(g_firstInstanceIdx)][patchIdx + 1],
            g_indexBuffers[NonUniformResourceIndex(g_firstInstanceIdx)][patchIdx + 2]
        };
        float3 firstVertices[] =
        {
            g_vertexBuffers[NonUniformResourceIndex(g_firstInstanceIdx)][firstIndices[0]],
            g_vertexBuffers[NonUniformResourceIndex(g_firstInstanceIdx)][firstIndices[1]],
            g_vertexBuffers[NonUniformResourceIndex(g_firstInstanceIdx)][firstIndices[2]]
        };
        float3 hitObjPos = firstVertices[0] * u + firstVertices[1] * v + firstVertices[2] * w;
        float4 hitPos4 = mul(float4(hitObjPos, 1.0), g_worlds[g_firstInstanceIdx]);
        float3 hitPos = hitPos4.xyz / hitPos4.w;

        float3 color = traceRadianceRay(rayOrigin, normalize(hitPos - rayOrigin), 0);
    
        g_tessColors[g_firstInstanceIdx][vertIdx] = color;
    }
    else
    {
        g_tessColors[g_firstInstanceIdx][vertIdx] = float3(0, 0, 0);
    }
}

[shader("closesthit")]
void closestHitRadiance(
    inout RayPayload payload,
    TriAttributes attr)
{
    Vertex input = getInput(attr.barycentrics);
    
    uint instanceIdx = InstanceIndex();
    float3 normal = normalize(mul(input.Norm, g_worldITs[instanceIdx]));
    float3 hitPos = hitWorldPosition();

    float3 shadowDirection = normalize(getLightPos() - hitPos);
    bool inShadow = traceShadowRay(hitPos, shadowDirection, payload.RecursionDepth);
    float3 reflectDirection = reflect(WorldRayDirection(), normal);

    float4 albedo = getAlbedo(instanceIdx);
    float3 reflColor = albedo.z * traceRadianceRay(hitPos, reflectDirection, payload.RecursionDepth);
    float3 otherColor =
        simpleLighting(
            hitPos,
            normal,
            getLightPos(),
            inShadow,
            albedo,
            getColor(instanceIdx),
            getSpecularExponent(instanceIdx)
        );

    payload.Color = reflColor + otherColor;
}

// This shader is skipped by setting RAY_FLAG_SKIP_CLOSEST_HIT_SHADER in TraceRay
[shader("closesthit")]
void closestHitShadow(
    inout ShadowPayload payload,
    TriAttributes attr)
{
}

[shader("miss")]
void missRadiance(
    inout RayPayload payload)
{
    payload.Color = environment(WorldRayDirection());
}

[shader("miss")]
void missShadow(
    inout ShadowPayload payload)
{
    payload.hit = false;
}
