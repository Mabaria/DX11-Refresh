#pragma once
#include "Fbx_loader.h"

class MeshObject {
private:
	std::vector<DirectX::XMFLOAT3>* mpVertexPosVector = nullptr;
	std::vector<int>* mpIndexVector = nullptr;

	// Used for skinning / skeletal animation
	/*Skeleton skeleton;
	std::unordered_map<int, ControlPointInfo> controlPointsInfo;*/

public:
	MeshObject();
	~MeshObject();
	HRESULT LoadFBX(const std::string& filePath);
	std::vector<DirectX::XMFLOAT3>* GetVertexPositionVector();
	std::vector<int>* GetIndexVector();
};