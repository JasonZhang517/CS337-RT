struct HSControlOut
{
    float3 Pos : Position;
};

struct HSConstOut
{
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor  : SV_InsideTessFactor;
};

cbuffer cbTessellation : register(b0)
{
    uint g_instanceIdx;
    uint g_tessFactor;
    uint g_maxVertPerPatch;
};

inline uint domainHash(float2 domain, uint tessFactor, uint maxVertPerPatch)
{
    float hash = (domain.x * tessFactor * tessFactor + domain.y) * tessFactor;
    return uint(hash * 16381) % maxVertPerPatch;
}
