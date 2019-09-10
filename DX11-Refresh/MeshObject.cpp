#include "MeshObject.h"
// Running this constructor implies you want an empty mesh object
MeshObject::MeshObject()
{

}

MeshObject::~MeshObject()
{
	delete mpVertexPosVector;
	delete mpIndexVector;
}

HRESULT MeshObject::LoadFBX(const std::string& filePath)
{
	if (!this->mpIndexVector || !this->mpVertexPosVector)
	{
		this->mpIndexVector = new std::vector<int>;
		this->mpVertexPosVector = new std::vector<DirectX::XMFLOAT3>;
	}
	//FbxLoader::LoadFBX(filePath, this->mpVertexVector, this->mpIndexVector, &this->skeleton);
	return FbxLoader::LoadFBX(filePath, this->mpVertexPosVector, this->mpIndexVector);
}

std::vector<DirectX::XMFLOAT3>* MeshObject::GetVertexPositionVector()
{
	return this->mpVertexPosVector;
}

std::vector<int>* MeshObject::GetIndexVector()
{
	return this->mpIndexVector;
}
