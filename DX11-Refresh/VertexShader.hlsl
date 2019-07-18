cbuffer wvp
{
	float4x4 gWorldViewProj;
};

struct VSIn
{
	float3 Pos	 : POSITION;
	float3 Normal : NORMAL;
	float2 UV		: TEXCOORD;
};

struct VSOut
{
	float4 Pos	 : SV_POSITION;
	float4 Color : COLOR;
};

VSOut VS(VSIn input)
{
	VSOut output;
	output.Pos = mul(float4(input.Pos, 1.0f), gWorldViewProj);
	output.Color = float4(0.83f, 0.83f, 0.83f, 1.0f);

	return output;
}