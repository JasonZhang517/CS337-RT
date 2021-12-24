struct VSOut
{
    float3 Pos : Position;
};

struct HSControlOut
{
    float3 Pos : Position;
};

struct HSConstOut
{
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};