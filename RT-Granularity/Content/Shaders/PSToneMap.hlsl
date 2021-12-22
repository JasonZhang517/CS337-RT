//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
Texture2D<float3> g_txSource;

//--------------------------------------------------------------------------------------
// Pixel shader
//--------------------------------------------------------------------------------------
min16float4 main(float4 Pos : SV_POSITION) : SV_TARGET
{
    const uint2 pos = Pos.xy;

    const float3 center = g_txSource[pos];
    const float3 left = g_txSource[uint2(pos.x - 1, pos.y)];
    const float3 right = g_txSource[uint2(pos.x + 1, pos.y)];
    const float3 up = g_txSource[uint2(pos.x, pos.y - 1)];
    const float3 down = g_txSource[uint2(pos.x, pos.y + 1)];

    min16float3 colors[5];
    colors[0] = min16float3(center);
    colors[1] = min16float3(left);
    colors[2] = min16float3(right);
    colors[3] = min16float3(up);
    colors[4] = min16float3(down);

    // Tone mapping
    [unroll]
    for (uint i = 0; i < 5; ++i)
        colors[i] /= colors[i] + 0.5;

    // Unsharp
    min16float3 laplace = -4.0 * colors[0];
    [unroll]
    for (i = 1; i < 5; ++i)
        laplace += colors[i];

    colors[0] -= 0.2 * laplace;

    return min16float4(colors[0], 1.0);
}
