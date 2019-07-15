cbuffer wvp
{
	float4x4 gWorldViewProj;
};

struct VSIn
{
	float3 Pos	 : POSITION;
	float4 Color : COLOR;
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
	output.Color = input.Color;

	return output;
}