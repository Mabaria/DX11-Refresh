#pragma once

#include <fbxsdk.h>
#include <vector>
#include <d3d11.h>
#include <cassert>
#include <map>
#include <unordered_map>
#include <DirectXMath.h>
#include <memory>


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



struct ControlPointInfo
{
	std::vector<IndexWeightPair> weightPairs;
};



//This stores the information of each key frame of each Joint
struct KeyFrame {
	FbxLongLong frameNum;
	FbxAMatrix globalTransform;	//transform matrix
	KeyFrame* next;
	KeyFrame() :
		next(nullptr)
	{}
};

struct Joint {
	FbxString jointName;
	int currentIndex;	//index of current joint	
	int parentIndex;	//index to its parent joint
	FbxAMatrix globalMatrix;
	FbxAMatrix localMatrix;
	KeyFrame* animation;
	FbxNode* mNode;

	FbxAMatrix mGlobalBindposeInverse;

	Joint() :
		animation(nullptr),
		mNode(nullptr)
	{
		localMatrix.SetIdentity();
		globalMatrix.SetIdentity();
		parentIndex = -1;
	}
};

struct Skeleton {
	std::vector<Joint> joints;
};

	//void DisplayHierarchy(FbxNode* node, int depth, int currIndex, int parentIndex);

	//void DisplayHierarchy(FbxScene* pScene);

	// Used for loading the very basics of an FBX
	// Input: std::string file name of FBX file, pointers to std::vectors to append the data to
	// Output: Appends XMFLOAT3 vertex positions and int indices to provided vectors
	HRESULT LoadFBX(const std::string& fileName, std::vector<DirectX::XMFLOAT3>* pOutVertexPosVector, std::vector<int>* pOutIndexVector,
		std::vector<DirectX::XMFLOAT3>* pOutNormalVector, std::vector<DirectX::XMFLOAT2>* pOutUVVector, Skeleton* pOutSkeleton, std::vector<ControlPointInfo>* pOutCPInfoVector);

	//HRESULT LoadFBX(const std::string& filename, std::vector<FbxVertex>* pOutVertexVector, std::vector<int>* pOutIndexVector, Skeleton* skeleton);


}