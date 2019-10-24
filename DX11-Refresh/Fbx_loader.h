#pragma once

#include <fbxsdk.h>
#include <vector>
#include <d3d11.h>
#include <cassert>
#include <map>
#include <unordered_map>
#include <DirectXMath.h>
#include <memory>
#include <algorithm>
#include <locale>

#define MAX_NUM_WEIGHTS_PER_VERTEX 4


namespace FbxLoader
{

struct FbxVertex
{
	float pos[3];
	float nor[3];
	float uv[2];
};

//This stores the weight of each Control Point
struct IndexWeightPair
{
	unsigned int index;	//index of joint 
	double weight;		//weight of influence by the joint
	IndexWeightPair() :
		index(0), weight(0.0)
	{}
};


// Each vertex contains up to four IndexWeightPairs
struct ControlPointInfo
{
	IndexWeightPair weightPairs[4];
};

//This stores the information of each key frame of each Joint
struct KeyFrame {
	FbxLongLong mFrameNum;
	FbxAMatrix mGlobalTransform;	//global transform not including bindpose inverse transform matrix
	FbxAMatrix mLocalTransform;
	DirectX::XMFLOAT4X4 mOffsetMatrix; // final offset matrix, this is what needs to be sent to the GPU
};

struct Joint {
	std::string mName;
	int mParentIndex;	//index to its parent joint

	std::vector<KeyFrame> mAnimationVector;
	FbxNode* mNode;

	std::vector<unsigned int> mConnectedVertexIndices;

	FbxAMatrix mGlobalBindposeInverse;
	FbxAMatrix mBoneGlobalTransform;
	FbxAMatrix mOffsetMatrix;

	Joint() :
		mNode(nullptr)
	{
		mParentIndex = -1;
	}
};

enum ANIMATION_TYPE
{
	IDLE,
	MOVE,
	ATTACK,
	ANIMATION_COUNT
};

struct AnimationSet
{
	DirectX::XMFLOAT4X4* animationData;
	unsigned int frameCount;
	unsigned int activeFrame = 0;
	std::string animationName;
};

struct Skeleton {
private:  
	float mCurrentTime = 0;

public:
	std::vector<Joint> joints;
	DirectX::XMFLOAT4X4* animationData; // LEGACY, please use UpdateAnimation and frameData instead
	DirectX::XMFLOAT4X4* frameData;
	unsigned int jointCount = 0;
	unsigned int frameCount = 0;
	AnimationSet animations[ANIMATION_COUNT];
	int animationFlags[ANIMATION_COUNT]; // -1 : missing animation
										 // 0  : disabled
										 // 1  : enabled
	Skeleton() 
	{
		// Initialize the animation flags with -1 for missing animation
		std::fill(animationFlags, animationFlags + ANIMATION_COUNT, -1);
	}
	// Currently does not blend animations, simply updates the global animationData with info from the first enabled animation.
	void UpdateAnimation(float dtInSeconds)
	{

		for (int i = 0; i < ANIMATION_COUNT; ++i)
		{
			if (this->animationFlags[i] == 1)
			{
				this->animationData = animations[i].animationData;
				this->frameCount = animations[i].frameCount;
				this->mCurrentTime = this->mCurrentTime + dtInSeconds;
				int frame_to_set = (int)std::round(fmod(this->mCurrentTime * 24.0f, this->frameCount)) % this->frameCount;
				// Get frame data from animationData vector
				memcpy(this->frameData, &animations[i].animationData[frame_to_set * this->jointCount], this->jointCount * sizeof(DirectX::XMFLOAT4X4));
				break; // This break will disappear when animation blending is implemented
			}
		}
	}
	// Returns false if requested animation does not exist
	bool StartAnimation(ANIMATION_TYPE anim_type)
	{
		if (anim_type < ANIMATION_COUNT)
		{
			if (this->animationFlags[anim_type] != -1)
			{
				this->animationFlags[anim_type] = 1;
				this->animations[anim_type].activeFrame = 0;
				return true;
			}
		}
		return false;
	}
	// Returns false if requested animation does not exist
	bool StopAnimation(ANIMATION_TYPE anim_type)
	{
		if (anim_type < ANIMATION_COUNT)
		{
			if (this->animationFlags[anim_type] != -1)
			{
				this->animationFlags[anim_type] = 0;
				return true;
			}
		}
		return false;
	}
};


	// Used for loading the very basics of an FBX
	// Input: std::string file name of FBX file, pointers to std::vectors to append the data to
	// Output: Appends XMFLOAT3 vertex positions and int indices to provided vectors
	HRESULT LoadFBX(const std::string& fileName, std::vector<DirectX::XMFLOAT3>* pOutVertexPosVector, std::vector<int>* pOutIndexVector,
		std::vector<DirectX::XMFLOAT3>* pOutNormalVector, std::vector<DirectX::XMFLOAT2>* pOutUVVector, Skeleton* pOutSkeleton, std::vector<ControlPointInfo>* pOutCPInfoVector);


}