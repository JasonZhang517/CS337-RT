struct PSIn
{
    float4 Pos   : SV_Position;
    float3 Color : Color;
};

RWTexture2D<float3> g_outputView : register(u0);

[earlydepthstencil]
void main(PSIn input)
{
    float2 pos = input.Pos.xy;
    g_outputView[pos] = input.Color;
}