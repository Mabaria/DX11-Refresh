#include "Fbx_loader.h"

class Mesh_Object {
private:
	std::vector<FbxVertex>* pVertexVector;
	std::vector<int>* pIndexVector;


public:
	Mesh_Object();
	~Mesh_Object();
	void LoadFBX(const std::string& filePath);
};