struct Vertex
{
    float3 Pos : Position;
    float3 Norm : Normal;
};

float3 main(Vertex input) : Position
{
    return input.Pos;
}