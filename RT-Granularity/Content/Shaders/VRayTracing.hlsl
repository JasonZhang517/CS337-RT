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

//[shader("closesthit")]
//void closestHitRadiance(
//    inout RayPayload payload,
//    TriAttributes    attr)
//{
//    Vertex input = getInput(attr.barycentrics);
    
//    uint instanceIdx = InstanceIndex();
//    float3 normal = normalize(mul(input.Norm, g_worldITs[instanceIdx]));
//    float3 hitPos = hitWorldPosition();

//    float3 shadowDirection = normalize(getLightPos() - hitPos);
//    bool inShadow = traceShadowRay(hitPos, shadowDirection, payload.RecursionDepth);
//    float3 reflectDirection = reflect(WorldRayDirection(), normal);

//    float4 albedo = getAlbedo(instanceIdx);
//    float3 reflColor = albedo.z * traceRadianceRay(hitPos, reflectDirection, payload.RecursionDepth);
//    float3 otherColor = 
//        simpleLighting(
//            hitPos, 
//            normal, 
//            getLightPos(), 
//            inShadow, 
//            albedo, 
//            getColor(instanceIdx), 
//            getSpecularExponent(instanceIdx)
//        );

//    payload.Color = reflColor + otherColor;
//}

//// This shader is skipped by setting RAY_FLAG_SKIP_CLOSEST_HIT_SHADER in TraceRay
//[shader("closesthit")]
//void closestHitShadow(
//    inout ShadowPayload payload,
//    TriAttributes       attr)
//{
//}

//[shader("miss")]
//void missRadiance(
//    inout RayPayload payload)
//{
//    payload.Color = environment(WorldRayDirection());
//}

//[shader("miss")]
//void missShadow(
//    inout ShadowPayload payload)
//{
//    payload.hit = false;
//}
