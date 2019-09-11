struct VSOut
{
	float4 Pos	 : SV_POSITION;
	float4 Color : COLOR;
	float2 UV	 : TEXCOORD;
	float3 worldPos : POSITION;
};

struct PSOut
{
	float4 Color;
};

Texture2D ObjTexture;
SamplerState MeshTextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

PSOut PS(VSOut input) : SV_Target
{
	PSOut output;
	output.Color = input.Color;
	if (input.UV.x > -0.5)
	{
		output.Color = ObjTexture.Sample(MeshTextureSampler, input.UV);
	}
	else
	{
		//output.Color = float4((input.worldPos + 6.0f) / 12.0f, 1.0f);
		output.Color = float4(input.worldPos, 1.0f);
		
	}

	return output;
}