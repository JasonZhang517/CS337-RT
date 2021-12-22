typedef RaytracingAccelerationStructure       RaytracingAS;
typedef BuiltInTriangleIntersectionAttributes TriAttributes;

#define MAX_RECURSION_DEPTH 1

struct Vertex
{
    float3 Pos;
    float3 Norm;
};

struct RayPayload
{
    float3 Color;
};

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
cbuffer cbRayGenConstants : register(b0)
{
    matrix l_projToWorld;
    float3 l_eyePt;
};

//--------------------------------------------------------------------------------------
// Texture and buffers
//--------------------------------------------------------------------------------------
RWTexture2D<float3>         g_outputView      : register(u0);
RaytracingAS                g_scene           : register(t0);
TextureCube<float3>         g_txEnv           : register(t1);
StructuredBuffer<float3>    g_vertexColors[]  : register(t2);

// IA buffers
Buffer<uint>                g_indexBuffers[]  : register(t0, space1);
StructuredBuffer<Vertex>    g_vertexBuffers[] : register(t0, space2);

//--------------------------------------------------------------------------------------
// Samplers
//--------------------------------------------------------------------------------------
SamplerState g_sampler : register(s0);

float3 environment(float3 dir, float3 ddx = 0.0, float3 ddy = 0.0)
{
    return ((abs(ddx) + abs(ddy) > 0.0 ? g_txEnv.SampleGrad(g_sampler, dir, ddx, ddy) :
        g_txEnv.SampleLevel(g_sampler, dir, 0.0)));
}

float3 getInput(float2 barycentrics)
{
    const uint meshIdx = InstanceIndex();
    const uint baseIdx = PrimitiveIndex() * 3;
    const uint3 indices =
    {
        g_indexBuffers[NonUniformResourceIndex(meshIdx)][baseIdx],
        g_indexBuffers[NonUniformResourceIndex(meshIdx)][baseIdx + 1],
        g_indexBuffers[NonUniformResourceIndex(meshIdx)][baseIdx + 2]
    };
    
    float3 colors[3] =
    {
        g_vertexColors[NonUniformResourceIndex(meshIdx)][indices[0]],
        g_vertexColors[NonUniformResourceIndex(meshIdx)][indices[1]],
        g_vertexColors[NonUniformResourceIndex(meshIdx)][indices[2]]
    };

    const float3 baryWeights =
    {
        1.0 - (barycentrics.x + barycentrics.y),
        barycentrics.xy
    };
        
    return
        baryWeights.x * colors[0] +
        baryWeights.y * colors[1] +
        baryWeights.z * colors[2];
}

float3 traceTheRay(
    float3 rayOrigin,
    float3 rayDirection,
    uint   currentDepth)
{
    if (currentDepth >= MAX_RECURSION_DEPTH)
    {
        return environment(WorldRayDirection());
    }
    
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDirection;
    ray.TMin = 0;
    ray.TMax = 1000;

    RayPayload payload = { float3(0, 0, 0) };
    TraceRay(g_scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        ~0,
        0, 1, 0,
        ray, payload);

    return payload.Color;
}

//--------------------------------------------------------------------------------------
// Ray generation
//--------------------------------------------------------------------------------------
[shader("raygeneration")]
void theRaygenMain()
{
    uint2 index = DispatchRaysIndex().xy;
    
    float3 rayOrigin = l_eyePt;
    
    float2 screenPos = (index + 0.5) / DispatchRaysDimensions().xy * 2.0 - 1.0;
    screenPos.y = -screenPos.y; // Invert Y for Y-up-style NDC.
    float4 world = mul(float4(screenPos, 0.0, 1.0), l_projToWorld);
    float3 hitPos = world.xyz / world.w;
    float3 rayDirection = normalize(hitPos - l_eyePt);
    
    float3 color = traceTheRay(rayOrigin, rayDirection, 0);

    g_outputView[index] = color;
}

[shader("closesthit")]
void theClosestHit(
    inout RayPayload payload,
    TriAttributes attr)
{
    payload.Color = getInput(attr.barycentrics);
}


[shader("miss")]
void theMissHit(
    inout RayPayload payload)
{
    payload.Color = environment(WorldRayDirection());
}
