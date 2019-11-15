struct VSOut
{
	float4 Pos	 : SV_POSITION;
	float4 Color : COLOR;
	float2 UV	 : TEXCOORD0;
	float3 worldPos : POSITION0;
};

struct PSOut
{
	float4 Color;
};

Texture2D<uint4> StencilTexture;

PSOut PS(VSOut input) : SV_Target
{
	PSOut output;
	int stencil_sum = 0;

	stencil_sum -= 1024 * StencilTexture.Load(int3(int2(input.Pos.x, input.Pos.y), 0)).y;
	stencil_sum += StencilTexture.Load(int3(int2(input.Pos.x + 3.0f, input.Pos.y + 3.0f), 0)).y;
	stencil_sum += StencilTexture.Load(int3(int2(input.Pos.x + 3.0f, input.Pos.y - 3.0f), 0)).y;
	stencil_sum += StencilTexture.Load(int3(int2(input.Pos.x - 3.0f, input.Pos.y + 3.0f), 0)).y;
	stencil_sum += StencilTexture.Load(int3(int2(input.Pos.x - 3.0f, input.Pos.y - 3.0f), 0)).y;

	stencil_sum += StencilTexture.Load(int3(int2(input.Pos.x + 3.0f, input.Pos.y + 0.0f), 0)).y;
	stencil_sum += StencilTexture.Load(int3(int2(input.Pos.x + 0.0f, input.Pos.y - 3.0f), 0)).y;
	stencil_sum += StencilTexture.Load(int3(int2(input.Pos.x - 0.0f, input.Pos.y + 3.0f), 0)).y;
	stencil_sum += StencilTexture.Load(int3(int2(input.Pos.x - 3.0f, input.Pos.y - 0.0f), 0)).y;

	float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (stencil_sum >= 105)
	{
		finalColor = float4(0.5f, 0.8f, 0.0f, 1.0f);
	}
	else if (stencil_sum >= 26)
	{
		finalColor = float4(0.5f, 0.2f, 0.9f, 1.0f);
	}
	else if (stencil_sum >= 5)
	{
		finalColor.x = 1.0f;
	}
	else if (stencil_sum > 0)
	{
		finalColor.y = 1.0f;
	}


	output.Color = finalColor;

	if (stencil_sum > 0)
		return output;

	discard;
	return output;
}