#include "Mesh_Object.h"
// Running this constructor implies you want an empty mesh object
// Thus it creates the vectors ready to be filled by a load
Mesh_Object::Mesh_Object()
{

}

Mesh_Object::~Mesh_Object()
{
}

void Mesh_Object::LoadFBX(const std::string& filePath)
{
	if (!this->pIndexVector || !this->pVertexVector)
	{
		this->pIndexVector = new std::vector<int>;
		this->pVertexVector = new std::vector<FbxVertex>;
	}
	FbxLoader::LoadFBX(filePath, this->pVertexVector, this->pIndexVector, &this->skeleton);
}
