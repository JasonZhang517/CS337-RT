#include "RTCommon.hlsli"

//--------------------------------------------------------------------------------------
// Texture and buffers
//--------------------------------------------------------------------------------------
RWTexture2D<float3>      g_outputView      : register(u0);

//--------------------------------------------------------------------------------------
// Ray generation
//--------------------------------------------------------------------------------------
[shader("raygeneration")]
void raygenMain()
{
    uint2 index = DispatchRaysIndex().xy;

    float3 rayOrigin = l_eyePt;
    
    float2 screenPos = (index + 0.5) / DispatchRaysDimensions().xy * 2.0 - 1.0;
    screenPos.y = -screenPos.y; // Invert Y for Y-up-style NDC.
    float4 world = mul(float4(screenPos, 0.0, 1.0), l_projToWorld);
    float3 hitPos = world.xyz / world.w;
    float3 rayDirection = normalize(hitPos - l_eyePt);
    
    float3 color = traceRadianceRay(rayOrigin, rayDirection, 0);

    g_outputView[index] = color;
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
