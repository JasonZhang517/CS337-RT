struct HSControlOut
{
    float3 Pos  : Position;
};

struct HSConstOut
{
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor  : SV_InsideTessFactor;
    uint  PatchID;
};

inline uint calcMaxVertPerPatch(uint tessFactor)
{
    const uint k = tessFactor / 2 + 1;
    return 3 * k * (tessFactor & 1 ? k + 1 : k);
};

inline uint domainHash(float2 domain, uint tessFactor)
{
    const uint N = calcMaxVertPerPatch(tessFactor);
    return (tessFactor * (domain.x * tessFactor + domain.y)) % N;
}

cbuffer cbTessellation : register(b0)
{
    uint g_instanceIdx;
    uint g_tessFactor;
};

RWStructuredBuffer<float2> g_tessDomains[] : register(u0);

[domain("tri")]
void main(
    HSConstOut input,
    float3 domain : SV_DomainLocation,
    const OutputPatch<HSControlOut, 3> patch)
{
    g_tessDomains[g_instanceIdx][domainHash(domain.xy, g_tessFactor)] = domain.xy;
}
