#include "Fbx_loader.h"

class Mesh_Object {
private:
	std::vector<FbxVertex>* pVertexVector = nullptr;
	std::vector<int>* pIndexVector = nullptr;

	// Used for skinning / skeletal animation
	Skeleton skeleton;
	std::unordered_map<int, ControlPointInfo> controlPointsInfo;

public:
	Mesh_Object();
	~Mesh_Object();
	void LoadFBX(const std::string& filePath);
};