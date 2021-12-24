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
