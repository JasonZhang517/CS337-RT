struct PSIn
{
    float4 Pos   : SV_Position;
    float3 Color : Color;
};

cbuffer cbEnv : register(b0)
{
    matrix g_projToWorld;
    float3 g_eyePt;
    float2 g_viewPort;
};

RWTexture2D<float3> g_outputView : register(u0);

[earlydepthstencil]
void main(PSIn input)
{
    float2 pos = input.Pos.xy;
    g_outputView[pos] = input.Color;
}