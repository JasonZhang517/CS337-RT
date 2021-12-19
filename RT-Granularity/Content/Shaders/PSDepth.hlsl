struct PSIn
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
};

float4 main(PSIn input) : SV_TARGET
{
    return input.Pos;
}
