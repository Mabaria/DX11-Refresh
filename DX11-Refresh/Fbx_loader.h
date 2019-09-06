#include <fbxsdk.h>
#include <vector>
#include <d3d11.h>
#include <cassert>
#include <unordered_map>

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
	FbxVector4 ctrlPoint;
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
	FbxNode* node;

	Joint() :
		animation(nullptr),
		node(nullptr)
	{
		localMatrix.SetIdentity();
		globalMatrix.SetIdentity();
		parentIndex = -1;
	}
};

struct Skeleton {
	std::vector<Joint> joints;
};

namespace FbxLoader
{

	static FbxManager* g_pFbxSdkManager = nullptr;
	static Skeleton skeleton;


	void DisplayHierarchy(FbxNode* node, int depth, int currIndex, int parentIndex);

	void DisplayHierarchy(FbxScene* pScene);

	HRESULT LoadFBX(const std::string& filename, std::vector<FbxVertex>* pOutVertexVector, std::vector<int>* pOutIndexVector);

	HRESULT LoadFBX(const std::string& filename, std::vector<FbxVertex>* pOutVertexVector, std::vector<int>* pOutIndexVector, Skeleton* skeleton);

}