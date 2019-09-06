#include "Fbx_Loader.h"

void FbxLoader::DisplayHierarchy(FbxNode* node, int depth, int currIndex, int parentIndex)
{
	FbxString jointName = node->GetName();
	//Display the hierarchy
	FbxString nodeNameBuf("");
	for (int i = 0; i != depth; ++i) {
		nodeNameBuf += "   ";
	}
	nodeNameBuf += jointName;
	nodeNameBuf += "\n";
	char buffer[100];
	sprintf_s(buffer, nodeNameBuf.Buffer());
	OutputDebugStringA(buffer);

	//if the type of current node is Skeleton, then it's a joint
	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
		Joint jointTmp;
		jointTmp.jointName = jointName;
		jointTmp.parentIndex = parentIndex;		//parent index
		jointTmp.currentIndex = currIndex;
		skeleton.joints.push_back(jointTmp);
	}

	//Display the hierarchy recursively
	for (int i = 0; i != node->GetChildCount(); ++i) {
		DisplayHierarchy(node->GetChild(i), depth + 1, skeleton.joints.size(), currIndex);
	}
}

void FbxLoader::DisplayHierarchy(FbxScene* pScene)
{
	OutputDebugStringA("\n\n---------------------------Hierarchy-------------------------------\n\n");
	FbxNode* rootNode = pScene->GetRootNode();
	int childCount = rootNode->GetChildCount();
	for (int i = 0; i != childCount; ++i) {
		DisplayHierarchy(rootNode->GetChild(i), 0, 0, -1);
	}
}

HRESULT FbxLoader::LoadFBX(const std::string& filename, std::vector<FbxVertex>* pOutVertexVector, std::vector<int>* pOutIndexVector)
{
	// Create the FbxManager if it does not already exist
	if (g_pFbxSdkManager == nullptr)
	{
		g_pFbxSdkManager = FbxManager::Create();

		FbxIOSettings* pIOsettings = FbxIOSettings::Create(g_pFbxSdkManager, IOSROOT);
		g_pFbxSdkManager->SetIOSettings(pIOsettings);
	}

	FbxImporter* pImporter = FbxImporter::Create(g_pFbxSdkManager, "");
	FbxScene* pFbxScene = FbxScene::Create(g_pFbxSdkManager, "");
	// Import model
	bool bSuccess = pImporter->Initialize(filename.c_str(), -1, g_pFbxSdkManager->GetIOSettings());
	if (!bSuccess) return E_FAIL;

	bSuccess = pImporter->Import(pFbxScene);
	if (!bSuccess) return E_FAIL;

	pImporter->Destroy();


	FbxNode* pFbxRootNode = pFbxScene->GetRootNode();

	DisplayHierarchy(pFbxScene);

	if (pFbxRootNode)
	{
		for (int i = 0; i < pFbxRootNode->GetChildCount(); i++)
		{
			FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;
			std::string nodeName = pFbxChildNode->GetName();
			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			std::unordered_map<int, ControlPointInfo> controlPointsInfo;

			// Handle Skeleton
			// Skeletons in the FBX are stored with a null type root node
			// Go one step deeper to reach the skeleton
			if (AttributeType == FbxNodeAttribute::eNull)
			{
				pFbxChildNode = pFbxChildNode->GetChild(0);
				AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();
				FbxSkeleton* pSkeleton = (FbxSkeleton*)pFbxChildNode->GetNodeAttribute();
				int numchildren = pFbxChildNode->GetChildCount();
				int k = 22;
			}

			// Handle Mesh attribute item
			else if (AttributeType == FbxNodeAttribute::eMesh)
			{


				FbxMesh* pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

				// Deformer is basically a skeleton, so there will most likely only be one deformer per mesh
				unsigned int numOfDeformers = pMesh->GetDeformerCount();
				for (unsigned int deformerIndex = 0; deformerIndex != numOfDeformers; ++deformerIndex)
				{
					FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
					// If skin not found

					if (!currSkin)
					{
						continue;
					}

					// Cluster == bone
					unsigned int numOfClusters = currSkin->GetClusterCount();
					for (unsigned int clusterIndex = 0; clusterIndex != numOfClusters; ++clusterIndex)
					{
						FbxCluster* currCluster = currSkin->GetCluster(clusterIndex);
						FbxString currJointName = currCluster->GetLink()->GetName();

						// Find joint index by name
						int currJointIndex = -1;
						for (int index = 0; index != skeleton.joints.size(); ++index) {
							if (skeleton.joints[index].jointName == currJointName)
							{
								currJointIndex = index;
								continue;
							}
						}
						if (currJointIndex == -1) {
							char buffer[100];
							sprintf_s(buffer, "Joint not found: %s\n\n", currJointName);
							OutputDebugStringA(buffer);
							continue;
						}

						FbxAMatrix localMatrix = currCluster->GetLink()->EvaluateLocalTransform();

						skeleton.joints[currJointIndex].node = currCluster->GetLink(); // Get current joint
						skeleton.joints[currJointIndex].localMatrix = localMatrix;

						// Parse vertex joint weights

						unsigned int numOfIndices = currCluster->GetControlPointIndicesCount();
						double* controlPointWeights = currCluster->GetControlPointWeights();
						int* controlPointIndices = currCluster->GetControlPointIndices();

						for (unsigned int i = 0; i != numOfIndices; ++i)
						{
							IndexWeightPair pair;
							pair.index = currJointIndex;
							pair.weight = controlPointWeights[i];

							controlPointsInfo[controlPointIndices[i]].weightPairs.push_back(pair);
							// SAVE TO CONTROL POINTS INFO STRUCT
						}

					}
				}

				FbxVector4* pVertices = pMesh->GetControlPoints();

				// Populate the index vector
				int* pIndices = pMesh->GetPolygonVertices();
				int indexCount = pMesh->GetPolygonVertexCount();
				pOutIndexVector->insert(pOutIndexVector->end(), &pIndices[0], &pIndices[indexCount]);

				for (int j = 0; j < pMesh->GetPolygonCount(); j++)
				{

					// Make sure the polygon is a triangle
					int iNumVertices = pMesh->GetPolygonSize(j);
					assert(iNumVertices == 3);

					for (int k = 0; k < iNumVertices; k++) {
						int iControlPointIndex = pMesh->GetPolygonVertex(j, k);

						FbxVertex vertex;
						vertex.pos[0] = (float)pVertices[iControlPointIndex].mData[0];
						vertex.pos[1] = (float)pVertices[iControlPointIndex].mData[1];
						vertex.pos[2] = (float)pVertices[iControlPointIndex].mData[2];

						fbxsdk::FbxVector4 nor;
						bool hasNor = pMesh->GetPolygonVertexNormal(j, k, nor);
						// Check if the polygon has vertex normals
						// if so then save in vertex
						if (hasNor)
						{
							vertex.nor[0] = (float)nor.mData[0];
							vertex.nor[1] = (float)nor.mData[1];
							vertex.nor[2] = (float)nor.mData[2];
						}

						// Extract UVs
						int UVSetCount = pMesh->GetUVLayerCount();
						if (UVSetCount > 0)
						{
							fbxsdk::FbxStringList UVSetNames;
							pMesh->GetUVSetNames(UVSetNames);
							fbxsdk::FbxVector2 UV;
							bool unmapped;
							pMesh->GetPolygonVertexUV(j, k, UVSetNames[0], UV, unmapped);
							vertex.uv[0] = (float)UV.mData[0];
							vertex.uv[1] = (float)UV.mData[1];
						}


						//bool hasUV = pMesh->GetPolygonVertexUV(j, k, )

						pOutVertexVector->push_back(vertex);
					}
				}
			}
		}

	}
	return S_OK;
}

HRESULT FbxLoader::LoadFBX(const std::string& filename, std::vector<FbxVertex>* pOutVertexVector, std::vector<int>* pOutIndexVector, Skeleton* skeleton)
{
	// Create the FbxManager if it does not already exist
	if (g_pFbxSdkManager == nullptr)
	{
		g_pFbxSdkManager = FbxManager::Create();

		FbxIOSettings* pIOsettings = FbxIOSettings::Create(g_pFbxSdkManager, IOSROOT);
		g_pFbxSdkManager->SetIOSettings(pIOsettings);
	}

	FbxImporter* pImporter = FbxImporter::Create(g_pFbxSdkManager, "");
	FbxScene* pFbxScene = FbxScene::Create(g_pFbxSdkManager, "");
	// Import model
	bool bSuccess = pImporter->Initialize(filename.c_str(), -1, g_pFbxSdkManager->GetIOSettings());
	if (!bSuccess) return E_FAIL;

	bSuccess = pImporter->Import(pFbxScene);
	if (!bSuccess) return E_FAIL;

	pImporter->Destroy();


	FbxNode* pFbxRootNode = pFbxScene->GetRootNode();

	DisplayHierarchy(pFbxScene);

	if (pFbxRootNode)
	{
		for (int i = 0; i < pFbxRootNode->GetChildCount(); i++)
		{
			FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;
			std::string nodeName = pFbxChildNode->GetName();
			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			std::unordered_map<int, ControlPointInfo> controlPointsInfo;

			// Handle Skeleton
			// Skeletons in the FBX are stored with a null type root node
			// Go one step deeper to reach the skeleton
			if (AttributeType == FbxNodeAttribute::eNull)
			{
				pFbxChildNode = pFbxChildNode->GetChild(0);
				AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();
				FbxSkeleton* pSkeleton = (FbxSkeleton*)pFbxChildNode->GetNodeAttribute();
				int numchildren = pFbxChildNode->GetChildCount();
				int k = 22;
			}

			// Handle Mesh attribute item
			else if (AttributeType == FbxNodeAttribute::eMesh)
			{


				FbxMesh* pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

				// Deformer is basically a skeleton, so there will most likely only be one deformer per mesh
				unsigned int numOfDeformers = pMesh->GetDeformerCount();
				for (unsigned int deformerIndex = 0; deformerIndex != numOfDeformers; ++deformerIndex)
				{
					FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
					// If skin not found

					if (!currSkin)
					{
						continue;
					}

					// Cluster == bone
					unsigned int numOfClusters = currSkin->GetClusterCount();
					for (unsigned int clusterIndex = 0; clusterIndex != numOfClusters; ++clusterIndex)
					{
						FbxCluster* currCluster = currSkin->GetCluster(clusterIndex);
						FbxString currJointName = currCluster->GetLink()->GetName();

						// Find joint index by name
						int currJointIndex = -1;
						for (int index = 0; index != skeleton->joints.size(); ++index) {
							if (skeleton->joints[index].jointName == currJointName)
							{
								currJointIndex = index;
								continue;
							}
						}
						if (currJointIndex == -1) {
							char buffer[100];
							sprintf_s(buffer, "Joint not found: %s\n\n", currJointName);
							OutputDebugStringA(buffer);
							continue;
						}

						FbxAMatrix localMatrix = currCluster->GetLink()->EvaluateLocalTransform();

						skeleton->joints[currJointIndex].node = currCluster->GetLink(); // Get current joint
						skeleton->joints[currJointIndex].localMatrix = localMatrix;

						// Parse vertex joint weights

						unsigned int numOfIndices = currCluster->GetControlPointIndicesCount();
						double* controlPointWeights = currCluster->GetControlPointWeights();
						int* controlPointIndices = currCluster->GetControlPointIndices();

						for (unsigned int i = 0; i != numOfIndices; ++i)
						{
							IndexWeightPair pair;
							pair.index = currJointIndex;
							pair.weight = controlPointWeights[i];

							controlPointsInfo[controlPointIndices[i]].weightPairs.push_back(pair);
							// SAVE TO CONTROL POINTS INFO STRUCT
						}

					}
				}

				FbxVector4* pVertices = pMesh->GetControlPoints();

				// Populate the index vector
				int* pIndices = pMesh->GetPolygonVertices();
				int indexCount = pMesh->GetPolygonVertexCount();
				pOutIndexVector->insert(pOutIndexVector->end(), &pIndices[0], &pIndices[indexCount]);

				for (int j = 0; j < pMesh->GetPolygonCount(); j++)
				{

					// Make sure the polygon is a triangle
					int iNumVertices = pMesh->GetPolygonSize(j);
					assert(iNumVertices == 3);

					for (int k = 0; k < iNumVertices; k++) {
						int iControlPointIndex = pMesh->GetPolygonVertex(j, k);

						FbxVertex vertex;
						vertex.pos[0] = (float)pVertices[iControlPointIndex].mData[0];
						vertex.pos[1] = (float)pVertices[iControlPointIndex].mData[1];
						vertex.pos[2] = (float)pVertices[iControlPointIndex].mData[2];

						fbxsdk::FbxVector4 nor;
						bool hasNor = pMesh->GetPolygonVertexNormal(j, k, nor);
						// Check if the polygon has vertex normals
						// if so then save in vertex
						if (hasNor)
						{
							vertex.nor[0] = (float)nor.mData[0];
							vertex.nor[1] = (float)nor.mData[1];
							vertex.nor[2] = (float)nor.mData[2];
						}

						// Extract UVs
						int UVSetCount = pMesh->GetUVLayerCount();
						if (UVSetCount > 0)
						{
							fbxsdk::FbxStringList UVSetNames;
							pMesh->GetUVSetNames(UVSetNames);
							fbxsdk::FbxVector2 UV;
							bool unmapped;
							pMesh->GetPolygonVertexUV(j, k, UVSetNames[0], UV, unmapped);
							vertex.uv[0] = (float)UV.mData[0];
							vertex.uv[1] = (float)UV.mData[1];
						}


						//bool hasUV = pMesh->GetPolygonVertexUV(j, k, )

						pOutVertexVector->push_back(vertex);
					}
				}
			}
		}

	}
	return S_OK;
}