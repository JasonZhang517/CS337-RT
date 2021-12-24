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
    if (domain.x > 0.8)
    {
        return 6;
    }
    else if (domain.y > 0.8)
    {
        return 4;
    }
    else if (domain.x + domain.y > 0.8)
    {
        return 5;
    }
    else if (domain.x + domain.y > 0.55)
    {
        return 3;
    }
    else if (domain.x + domain.y < 0.2)
    {
        return 0;
    }
    else if (domain.x < 0.1)
    {
        return 1;
    }
    else
        return 2;
}
