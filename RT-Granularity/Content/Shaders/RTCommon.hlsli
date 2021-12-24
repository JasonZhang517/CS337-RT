typedef RaytracingAccelerationStructure       RaytracingAS;
typedef BuiltInTriangleIntersectionAttributes TriAttributes;

#define NUM_MESH            2
#define HIT_GROUP_RADIANCE  0
#define HIT_GROUP_SHADOW    1
#define MAX_RECURSION_DEPTH 2

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct Vertex
{
    float3 Pos;
    float3 Norm;
};

struct RayPayload
{
    float3 Color;
    uint   RecursionDepth;
};

struct ShadowPayload
{
    bool   hit;
};

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
cbuffer cbMaterial : register(b0)
{
    float4 g_baseColor[NUM_MESH];
    float4 g_albedo[NUM_MESH];
};

cbuffer cbGlobalConstants : register(b1)
{
    float3x3 g_worldITs[NUM_MESH];
    float4x4 g_worlds[NUM_MESH];
};

cbuffer cbRayGenConstants : register(b2)
{
    matrix l_projToWorld;
    float3 l_eyePt;
};

//--------------------------------------------------------------------------------------
// Texture and buffers
//--------------------------------------------------------------------------------------
RaytracingAS        g_scene : register(t0);
TextureCube<float3> g_txEnv : register(t1);

// IA buffers
Buffer<uint>              g_indexBuffers[] : register(t0, space1);
StructuredBuffer<Vertex> g_vertexBuffers[] : register(t0, space2);

//--------------------------------------------------------------------------------------
// Samplers
//--------------------------------------------------------------------------------------
SamplerState g_sampler : register(s0);

float3 environment(float3 dir)
{
    return g_txEnv.SampleLevel(g_sampler, dir, 0.0);
}

Vertex getInput(float2 barycentrics)
{
    const uint meshIdx = InstanceIndex();
    const uint baseIdx = PrimitiveIndex() * 3;
    const uint3 indices =
    {
        g_indexBuffers[NonUniformResourceIndex(meshIdx)][baseIdx],
        g_indexBuffers[NonUniformResourceIndex(meshIdx)][baseIdx + 1],
        g_indexBuffers[NonUniformResourceIndex(meshIdx)][baseIdx + 2]
    };

    // Retrieve corresponding vertex normals for the triangle vertices.
    Vertex vertices[3] =
    {
        g_vertexBuffers[NonUniformResourceIndex(meshIdx)][indices[0]],
        g_vertexBuffers[NonUniformResourceIndex(meshIdx)][indices[1]],
        g_vertexBuffers[NonUniformResourceIndex(meshIdx)][indices[2]]
    };

    const float3 baryWeights =
    {
        1.0 - (barycentrics.x + barycentrics.y),
        barycentrics.xy
    };

    Vertex input;
    input.Pos =
        baryWeights.x * vertices[0].Pos +
        baryWeights.y * vertices[1].Pos +
        baryWeights.z * vertices[2].Pos;

    input.Norm =
        baryWeights.x * vertices[0].Norm +
        baryWeights.y * vertices[1].Norm +
        baryWeights.z * vertices[2].Norm;

    return input;
}

float4 getAlbedo(uint instanceIdx)
{
    return g_albedo[instanceIdx];
}

float3 getColor(uint instanceIdx)
{
    return g_baseColor[instanceIdx].xyz;
}

float getSpecularExponent(uint instanceIdx)
{
    return g_baseColor[instanceIdx].w;
}

float3 hitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

inline float calcDiffuseCoef(
    float3 incidentLightRay,
    float3 normal)
{
    float fNDotL = saturate(dot(-incidentLightRay, normal));
    return fNDotL;
}

inline float calcSpecularCoef(
    float3 incidentLightRay,
    float3 normal,
    float  specularPower)
{
    float3 reflectedLightRay = normalize(reflect(incidentLightRay, normal));
    return pow(saturate(dot(reflectedLightRay, normalize(-incidentLightRay))), specularPower);
}

float3 getLightPos()
{
    return l_eyePt + float3(20, 20, 0);
}

static const float InShadowRadiance = 0.35f;

float3 simpleLighting(
    float3 hitPos,
    float3 normal,
    float3 lightPos,
    bool inShadow,
    float4 albedo,
    float3 materialColor,
    float specularPower = 50)
{
    float shadowFactor = inShadow ? InShadowRadiance : 1.0;
    float3 incidentLightRay = normalize(hitPos - lightPos);

    // diffuse
    float kd = calcDiffuseCoef(incidentLightRay, normal);
    float3 diffuseColor = shadowFactor * kd * albedo.x * materialColor;
    
    // specular
    float3 specularColor = float3(0, 0, 0);
    if (!inShadow)
    {
        float ks = calcSpecularCoef(incidentLightRay, normal, specularPower);
        specularColor = ks * albedo.y * float3(1, 1, 1);
    }

    return diffuseColor + specularColor;
}

float3 traceRadianceRay(
    float3 rayOrigin,
    float3 rayDirection,
    uint   currentDepth)
{
    if (currentDepth >= MAX_RECURSION_DEPTH)
    {
        return environment(rayDirection);
    }
    
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDirection;
    ray.TMin = 0;
    ray.TMax = 1000;

    RayPayload payload = { float3(0, 0, 0), currentDepth + 1 };
    TraceRay(g_scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        ~0,
        HIT_GROUP_RADIANCE, 1, HIT_GROUP_RADIANCE,
        ray, payload);

    return payload.Color;
}

bool traceShadowRay(
    float3 rayOrigin,
    float3 rayDirection,
    uint   currentDepth)
{
    if (currentDepth >= MAX_RECURSION_DEPTH)
    {
        return false;
    }
    
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDirection;
    ray.TMin = 0;
    ray.TMax = 1000;
    
    ShadowPayload shadowPayload = { true };
    TraceRay(g_scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_FORCE_OPAQUE
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        ~0,
        HIT_GROUP_SHADOW, 1, HIT_GROUP_SHADOW,
        ray, shadowPayload);

    return shadowPayload.hit;
}

//--------------------------------------------------------------------------------------
// Hit shaders
//--------------------------------------------------------------------------------------
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
