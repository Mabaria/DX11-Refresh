struct SKYBOX_VS_OUTPUT    //output structure for SKYBOX vertex shader
{
	float4 Pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;
TextureCube SkyBox;

float4 SKYBOX_PS(SKYBOX_VS_OUTPUT input) : SV_Target
{
	return SkyBox.Sample(ObjSamplerState, input.texCoord);
}