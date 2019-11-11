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
//output.Color = input.Color;
//output.Color = ObjTexture.Sample(MeshTextureSampler, input.UV);
output.Color = float4(input.worldPos, 1.0f);

return output;
}