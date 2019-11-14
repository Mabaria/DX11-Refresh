struct VSOut
{
	float4 Pos	 : SV_POSITION;
	float4 Color : COLOR;
	float2 UV	 : TEXCOORD0;
	float3 worldPos : POSITION0;
	uint outlineStencilValue : STENCILVALUE;
};

struct PSOut
{
	float4 BackBuffer			: SV_TARGET0;
	uint OutlineBuffer			: SV_TARGET1;
};


PSOut PS(VSOut input)
{
	PSOut output;
	output.BackBuffer	= float4(input.Color.x, input.Color.x, input.Color.x, 1.0f);
	output.OutlineBuffer = input.outlineStencilValue;
return output;
}