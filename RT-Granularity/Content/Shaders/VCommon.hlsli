struct VSIn
{
    float3 Pos : Position;
    float3 Norm : Normal;
    uint vId : SV_VertexID;
};

struct VSOut
{
    float3 Pos : Position;
    float3 Color : Color;
};

struct HSControlOut
{
    float3 Pos : Position;
    float3 Color : Color;
};

struct HSConstOut
{
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};

struct PSIn
{
    float4 Pos : SV_Position;
    float3 Color : Color;
};
