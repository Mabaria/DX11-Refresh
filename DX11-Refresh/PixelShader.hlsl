struct VSOut
{
	float4 Pos	 : SV_POSITION;
	float4 Color : COLOR;
};

struct PSOut
{
	float4 Color;
};

PSOut PS(VSOut input) : SV_Target
{
	PSOut output;
	output.Color = input.Color;
	return output;
}