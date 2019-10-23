static const int MAX_AFFECTING_BONES = 4;
static const int MAX_BONE_MATRICES = 63;

cbuffer wvp : register(b0)
{
	float4x4 gWorldViewProj;
};

cbuffer bones : register(b1)
{
	float4x4 gBoneTransforms[MAX_BONE_MATRICES];
};

struct SKIN_VSIn
{
	float3 Pos	 : POSITION;
	float3 Normal : NORMAL;
	float2 UV		: TEXCOORD;
	float3 blendWeights : BLENDWEIGHT0;
	min16int4 blendIndices : BLENDINDICES0;
};

struct VSOut
{
	float4 Pos	 : SV_POSITION;
	float4 Color : COLOR;
	float2 UV	 : TEXCOORD0;
	float3 worldPos : POSITION0;
};


VSOut SKIN_VS(SKIN_VSIn input)
{
	VSOut output = (VSOut)0;

	float lastWeight = 1.0f;
	float4 v = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float3 norm = float3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < MAX_AFFECTING_BONES - 1; ++i)
	{
		v		+= input.blendWeights[i] * mul(float4(input.Pos, 1.0f), gBoneTransforms[input.blendIndices[i]]);
		norm	+= (input.blendWeights[i] * mul(float4(input.Normal, 1.0f), gBoneTransforms[input.blendIndices[i]])).xyz;
		lastWeight -= input.blendWeights[i];
	}
	// Apply last weight
	v += lastWeight * mul(float4(input.Pos, 1.0f), gBoneTransforms[input.blendIndices[MAX_AFFECTING_BONES - 1]]);
	norm += (lastWeight * mul(float4(input.Normal, 1.0f), gBoneTransforms[input.blendIndices[MAX_AFFECTING_BONES - 1]])).xyz;
	v.w = 1.0f;
	output.Pos = mul(v, gWorldViewProj);
	output.UV = input.UV;
	output.Color = float4(input.Normal, 1.0f);
	// Used for normal testing purposes to assign colour in the pixel shader
	output.worldPos = norm;
	return output;
}