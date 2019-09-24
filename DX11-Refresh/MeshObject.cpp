#include "MeshObject.h"
// Running this constructor implies you want an empty mesh object
MeshObject::MeshObject()
{

}

MeshObject::~MeshObject()
{
	delete mpVertexPosVector;
	delete mpIndexVector;
	delete mpNormalVector;
	delete mpUVVector;
}

HRESULT MeshObject::LoadFBX(const std::string& filePath)
{
	if (!this->mpIndexVector || !this->mpVertexPosVector || !this->mpNormalVector || !this->mpUVVector)
	{
		this->mpIndexVector = new std::vector<int>;
		this->mpVertexPosVector = new std::vector<DirectX::XMFLOAT3>;
		this->mpNormalVector = new std::vector<DirectX::XMFLOAT3>;
		this->mpUVVector = new std::vector<DirectX::XMFLOAT2>;
	}
	//FbxLoader::LoadFBX(filePath, this->mpVertexVector, this->mpIndexVector, &this->skeleton);
	// Assume UVs and normals exist
	HRESULT hr = E_FAIL;
	try
	{
		hr = FbxLoader::LoadFBX(filePath, this->mpVertexPosVector, this->mpIndexVector, this->mpNormalVector, this->mpUVVector);
	}
	catch (std::exception e)
	{
		MessageBoxA(NULL, e.what(), "Error in FBX Loader.", MB_OK);
	}
	
	if (SUCCEEDED(hr))
	{
		this->mHasUvs = true;
		this->mHasNormals = true;
		if (this->mpNormalVector->size() <= 0)
		{
			delete mpNormalVector;
			mpNormalVector = nullptr;
			this->mHasNormals = false;
		}
		if (this->mpUVVector->size() <= 0)
		{
			delete mpUVVector;
			mpUVVector = nullptr;
			this->mHasUvs = false;
		}
	}
	else
	{
		delete mpVertexPosVector;
		delete mpIndexVector;
		delete mpNormalVector;
		delete mpUVVector;
		mpVertexPosVector = nullptr;
		mpUVVector = nullptr;
		mpNormalVector = nullptr;
		mpIndexVector = nullptr;
	}
	
	return hr;
	
}

std::vector<DirectX::XMFLOAT3>* MeshObject::GetVertexPositionVector()
{
	return this->mpVertexPosVector;
}

std::vector<int>* MeshObject::GetIndexVector()
{
	return this->mpIndexVector;
}

std::vector<DirectX::XMFLOAT3>* MeshObject::GetNormalVector()
{
	return this->mpNormalVector;
}

std::vector<DirectX::XMFLOAT2>* MeshObject::GetUVVector()
{
	return this->mpUVVector;
}

bool MeshObject::HasUVs()
{
	return this->mHasUvs;
}

bool MeshObject::HasNormals()
{
	return this->mHasNormals;
}
