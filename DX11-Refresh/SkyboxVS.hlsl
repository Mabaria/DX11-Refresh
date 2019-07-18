cbuffer wvp
{
	float4x4 gWorldViewProj;
};

struct SKYBOX_VS_OUTPUT    //output structure for SKYBOX vertex shader
{
	float4 Pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
};

struct VSIn
{
	float3 Pos	 : POSITION;
	float3 Normal : NORMAL;
	float2 UV		: TEXCOORD;
};

SKYBOX_VS_OUTPUT SKYBOX_VS(VSIn input)
{
	SKYBOX_VS_OUTPUT output = (SKYBOX_VS_OUTPUT)0;
	// xyww, z always 1, furthest away
	output.Pos = mul(float4(input.Pos, 1.0f), gWorldViewProj).xyww;

	output.texCoord = input.Pos;

	return output;
}