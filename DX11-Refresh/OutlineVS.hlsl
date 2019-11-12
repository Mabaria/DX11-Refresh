


struct VSOut
{
	float4 Pos	 : SV_POSITION;
	float4 Color : COLOR;
	float2 UV	 : TEXCOORD0;
	float3 worldPos : POSITION0;
};

VSOut VS(uint VertexID: SV_VertexID)
{
	VSOut output = (VSOut)0;
	output.Pos = float4(float2(((VertexID << 1) & 2) * 2.0f, (VertexID == 0) * -4.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return output;
}