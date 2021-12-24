struct Vertex
{
    float3 Pos : Position;
    float3 Norm : Normal;
};

struct VSOut
{
    float3 Pos : Position;
};

VSOut main(Vertex input)
{
    VSOut output = { input.Pos };
    return output;
}