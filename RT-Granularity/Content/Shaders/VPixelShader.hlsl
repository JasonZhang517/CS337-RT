struct PSIn
{
    float4 Pos   : SV_Position;
    float3 Color : Color;
};

// RWTexture2D<float3> g_outputView : register(u0);

float4 main(PSIn input) : SV_Target
{
    // float2 pos = input.Pos.xy / input.Pos.w;
    // g_outputView[pos] = input.Color;
    return float4(input.Color, 1.0);
}