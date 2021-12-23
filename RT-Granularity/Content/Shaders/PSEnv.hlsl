cbuffer cbEnv : register(b0)
{
    matrix g_projToWorld;
    float3 g_eyePt;
    float2 g_viewPort;
};

RWTexture2D<float3> g_outputView : register(u0);
TextureCube<float3> g_txEnv      : register(t0);
SamplerState        g_sampler    : register(s0);

float3 environment(float3 dir)
{
    return g_txEnv.SampleLevel(g_sampler, dir, 0.0);
}

void main(float4 Pos : SV_Position)
{
    float2 pos = Pos.xy / Pos.w;
    float2 screenPos = pos / g_viewPort * 2.0 - 1.0;
    screenPos.y = -screenPos.y; // Invert Y for Y-up-style NDC.
    float4 world = mul(float4(screenPos, 0.0, 1.0), g_projToWorld);
    float3 color = environment(world.xyz / world.w - g_eyePt);
    
    g_outputView[pos] = color;
}