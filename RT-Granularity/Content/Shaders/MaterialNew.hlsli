//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define NUM_MESH 2

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register (b0)
{
	float4 g_baseColors[NUM_MESH];
	float2 g_roughMetals[NUM_MESH];
};

min16float4 getBaseColor(uint instanceIdx)
{
	return min16float4(g_baseColors[instanceIdx]);
}

min16float getRoughness(uint instanceIdx, min16float roughness)
{
	/*if (instanceIdx == 0)
	{
		uint2 p = uv * 5.0;
		p &= 0x1;
		roughness = p.x ^ p.y ? roughness : roughness;
	}

	return roughness;*/
	return instanceIdx == 0 ? 0 : roughness;
}

min16float2 getRoughMetal(uint instanceIdx)
{
	const float2 roughMetal = g_roughMetals[instanceIdx];
	const min16float roughness = getRoughness(instanceIdx, min16float(roughMetal.x));

	return min16float2(roughness, roughMetal.y);
}

min16float getRoughness(uint instanceIdx)
{
	const float roughness = g_roughMetals[instanceIdx].x;

	return getRoughness(instanceIdx, min16float(roughness));
}

min16float getMetallic(uint instanceIdx)
{
	return min16float(g_roughMetals[instanceIdx].y);
}
