#include "Fbx_Loader.h"

// Anonymous namespace for Fbx_Loader
namespace {
	static FbxManager* gpFbxSdkManager = nullptr;
	static FbxLoader::Skeleton skeleton;
	void LoadUV(fbxsdk::FbxMesh* pMesh, std::vector<DirectX::XMFLOAT2>* pOutUVVector)
	{
		fbxsdk::FbxStringList uv_set_name_list;
		pMesh->GetUVSetNames(uv_set_name_list);

		// Only supporting one UV set, get the first in the list
		const char* uv_set_name = uv_set_name_list.GetStringAt(0);
		fbxsdk::FbxGeometryElementUV* uv_element = pMesh->GetElementUV(uv_set_name);

		// if no UV found
		if (!uv_element)
			return;

		// only support mapping mode eByPolygonVertex and eByControlPoint
		if (uv_element->GetMappingMode() != FbxGeometryElement::eByPolygonVertex &&
			uv_element->GetMappingMode() != FbxGeometryElement::eByControlPoint)
			return;
		// if mode is not direct, it is index based
		const bool use_index = uv_element->GetReferenceMode() != FbxGeometryElement::eDirect;
		const int index_count = (use_index) ? uv_element->GetIndexArray().GetCount() : 0;

		const int poly_count = pMesh->GetPolygonCount();
		DirectX::XMFLOAT2 uv;

		// If uvs are stored per polygon vertex we need to convert them to per control point
		if (uv_element->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			// Store a list of control points we've already processed
			std::map<int, bool> processed_cp;
			fbxsdk::FbxArray<FbxVector2> new_uvs;
			FbxArray<int> new_index_to_direct;

			for (int i = 0; i < pMesh->GetControlPointsCount(); ++i)
				processed_cp.insert_or_assign(i, false);

			for (int polygon = 0; polygon < pMesh->GetPolygonCount(); ++polygon)
			{
				for (int polygon_vertex = 0; polygon_vertex < pMesh->GetPolygonSize(polygon); ++polygon_vertex)
				{
					int control_point = pMesh->GetPolygonVertex(polygon, polygon_vertex);
					// If control point not already processed
					if (processed_cp.find(control_point)->second == false)
					{
						FbxVector2 uv = uv_element->GetDirectArray().GetAt(control_point);
						new_uvs.Add(uv);
						new_index_to_direct.Add(control_point);
						processed_cp.insert_or_assign(control_point, true);
					}
				}
			}
			// Create the new index array
			uv_element->SetMappingMode(FbxLayerElement::eByControlPoint);
			uv_element->GetIndexArray().Clear();
			uv_element->GetIndexArray().Resize(new_index_to_direct.GetCount());
			int* index_array = (int*)uv_element->GetIndexArray().GetLocked();
			for (int i = 0; i < new_index_to_direct.GetCount(); i++)
			{
				index_array[i] = new_index_to_direct.GetAt(i);
			}
			uv_element->GetIndexArray().Release((void**)& index_array);

			// Create the new direct array
			uv_element->GetDirectArray().Clear();
			uv_element->GetDirectArray().Resize(new_uvs.GetCount());
			FbxVector2* direct_array = (FbxVector2*)uv_element->GetDirectArray().GetLocked();
			for (int j = 0; j < new_uvs.GetCount(); ++j)
			{
				direct_array[j] = new_uvs.GetAt(j);
			}
			uv_element->GetDirectArray().Release((void**)& direct_array);
		}
		// Process UVs
		if (uv_element->GetMappingMode() == FbxGeometryElement::eByControlPoint)
		{

			for (int i = 0; i < pMesh->GetControlPointsCount(); ++i)
			{
				switch (uv_element->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					uv.x = (float)uv_element->GetDirectArray().GetAt(i).mData[0];
					uv.y = (float)uv_element->GetDirectArray().GetAt(i).mData[1];
					break;
				case FbxGeometryElement::eIndexToDirect:
				{
					int index = uv_element->GetIndexArray().GetAt(i);
					uv.x = (float)uv_element->GetDirectArray().GetAt(index).mData[0];
					uv.y = (float)uv_element->GetDirectArray().GetAt(index).mData[1];
					break;
				}
				default:
					throw std::exception("Invalid Fbx UV Reference");
				}
				pOutUVVector->push_back(uv);
			}
		}
	}
}

/*
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
*/

HRESULT FbxLoader::LoadFBX(const std::string& fileName, std::vector<DirectX::XMFLOAT3>* pOutVertexPosVector, std::vector<int>* pOutIndexVector,
	std::vector<DirectX::XMFLOAT3>* pOutNormalVector, std::vector<DirectX::XMFLOAT2>* pOutUVVector)
{
	// Create the FbxManager if it does not already exist
	if (gpFbxSdkManager == nullptr)
	{
		gpFbxSdkManager = FbxManager::Create();

		FbxIOSettings* p_io_settings = FbxIOSettings::Create(gpFbxSdkManager, IOSROOT);
		gpFbxSdkManager->SetIOSettings(p_io_settings);
	}

	FbxImporter* p_importer = FbxImporter::Create(gpFbxSdkManager, "");
	FbxScene* p_fbx_scene = FbxScene::Create(gpFbxSdkManager, "");

	auto scene_deleter = [](FbxScene* scene) {scene->Destroy(); };
	std::unique_ptr<FbxScene, decltype(scene_deleter)> pFbxScene2(FbxScene::Create(gpFbxSdkManager, ""), scene_deleter);

	// Import model

	bool bSuccess = p_importer->Initialize(fileName.c_str(), -1, gpFbxSdkManager->GetIOSettings());
	if (!bSuccess) {
		FbxString error = p_importer->GetStatus().GetErrorString();
		OutputDebugStringA("error: Call to FbxImporter::Initialize() failed.\n");

		char buffer[100];
		sprintf_s(buffer, "error: Error returned: %s\n", error.Buffer());
		OutputDebugStringA(buffer);

		if (p_importer->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion || true) {
			int lFileMajor, lFileMinor, lFileRevision;
			int lSDKMajor, lSDKMinor, lSDKRevision;
			// Get the file version number generate by the FBX SDK.
			FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);
			p_importer->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);
			sprintf_s(buffer, "error: FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			OutputDebugStringA(buffer);

			sprintf_s(buffer, "error: FBX file format version for file '%s' is %d.%d.%d\n", fileName, lFileMajor, lFileMinor,
				lFileRevision);
			OutputDebugStringA(buffer);
		}
		sprintf_s(buffer, "error: Failed to initialize the importer for '%s'.\n", fileName);
		OutputDebugStringA(buffer);
		return E_FAIL;;
	}

	bSuccess = p_importer->Import(pFbxScene2.get());
	if (!bSuccess) return E_FAIL;

	// Importer is no longer needed, remove from memory
	p_importer->Destroy();


	FbxNode* p_fbx_root_node = pFbxScene2->GetRootNode();


	// Useful for skeleton/bone structure
	//DisplayHierarchy(p_fbx_scene);

	if (p_fbx_root_node)
	{
		for (int i = 0; i < p_fbx_root_node->GetChildCount(); i++)
		{
			FbxNode* pFbxChildNode = p_fbx_root_node->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;
			std::string nodeName = pFbxChildNode->GetName();
			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			std::unordered_map<int, ControlPointInfo> controlPointsInfo;

			// Handle Skeleton
			// Skeletons in the FBX are stored with a null type root node
			// Go one step deeper to reach the skeleton
			/*
			if (AttributeType == FbxNodeAttribute::eNull)
			{
				pFbxChildNode = pFbxChildNode->GetChild(0);
				AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();
				FbxSkeleton* pSkeleton = (FbxSkeleton*)pFbxChildNode->GetNodeAttribute();
				int num_children = pFbxChildNode->GetChildCount();
			}
			*/
			// Handle Mesh attribute item
			if (AttributeType == FbxNodeAttribute::eMesh)
			{


				FbxMesh* p_mesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

				// ---------- SKELETON RELATED CODE -----------------
				/*
				// Deformer is basically a skeleton, so there will most likely only be one deformer per mesh
				unsigned int numOfDeformers = p_mesh->GetDeformerCount();
				for (unsigned int deformerIndex = 0; deformerIndex != numOfDeformers; ++deformerIndex)
				{
					FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(p_mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
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
				*/

				// Make sure the polygon is a triangle
				assert(p_mesh->IsTriangleMesh() && "Mesh contains non-triangles, please triangulate the mesh.");

				FbxVector4* p_vertices = p_mesh->GetControlPoints();

				// Populate the index vector
				int* p_indices = p_mesh->GetPolygonVertices();
				int index_count = p_mesh->GetPolygonVertexCount();
				pOutIndexVector->insert(pOutIndexVector->end(), &p_indices[0], &p_indices[index_count]);

				//// Flip the winding order
				//for (auto it = pOutIndexVector->begin(); it != pOutIndexVector->end(); it += 3)
				//{
				//	std::swap(*it, *(it + 2));
				//}

				::LoadUV(p_mesh, pOutUVVector);

				for (int j = 0; j < p_mesh->GetControlPointsCount(); ++j)
				{
					DirectX::XMFLOAT3 vertex_pos;
					vertex_pos.x = (float)p_vertices[j].mData[0];
					vertex_pos.y = (float)p_vertices[j].mData[1];
					vertex_pos.z = (float)p_vertices[j].mData[2];
					pOutVertexPosVector->push_back(vertex_pos);


					fbxsdk::FbxVector4 normal;
					// If the mesh normals are not in "per vertex" mode, re-generate them to be useable by this parser
					if (p_mesh->GetElementNormal(0)->GetMappingMode() != FbxGeometryElement::eByControlPoint)
					{
						p_mesh->GenerateNormals(true, true, true);
					}
					// Only allowing one normal per element currently
					fbxsdk::FbxGeometryElementNormal* le_normal = p_mesh->GetElementNormal(0);
					switch (le_normal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						normal = le_normal->GetDirectArray().GetAt(j).mData;
						break;
					case FbxGeometryElement::eIndexToDirect:
					{
						int index = le_normal->GetIndexArray().GetAt(j);
						normal = le_normal->GetDirectArray().GetAt(index).mData;
						break;
					}
					default:
						throw std::exception("Invalid Fbx Normal Reference");
					}
					DirectX::XMFLOAT3 vertex_normal;
					normal.Normalize();
					vertex_normal.x = (float)normal.mData[0];
					vertex_normal.y = (float)normal.mData[1];
					vertex_normal.z = (float)normal.mData[2];
					pOutNormalVector->push_back(vertex_normal);

				}
			}
		}

	}
	return S_OK;
}

/* 
HRESULT FbxLoader::LoadFBX(const std::string& filename, std::vector<FbxVertex>* pOutVertexVector, std::vector<int>* pOutIndexVector, Skeleton* skeleton)
{
	// Create the FbxManager if it does not already exist
	if (g_pFbxSdkManager == nullptr)
	{
		g_pFbxSdkManager = FbxManager::Create();

		FbxIOSettings* p_io_settings = FbxIOSettings::Create(g_pFbxSdkManager, IOSROOT);
		g_pFbxSdkManager->SetIOSettings(p_io_settings);
	}

	FbxImporter* p_importer = FbxImporter::Create(g_pFbxSdkManager, "");
	FbxScene* p_fbx_scene = FbxScene::Create(g_pFbxSdkManager, "");
	// Import model
	bool bSuccess = p_importer->Initialize(filename.c_str(), -1, g_pFbxSdkManager->GetIOSettings());
	if (!bSuccess) return E_FAIL;

	bSuccess = p_importer->Import(p_fbx_scene);
	if (!bSuccess) return E_FAIL;

	p_importer->Destroy();


	FbxNode* p_fbx_root_node = p_fbx_scene->GetRootNode();

	DisplayHierarchy(p_fbx_scene);

	if (p_fbx_root_node)
	{
		for (int i = 0; i < p_fbx_root_node->GetChildCount(); i++)
		{
			FbxNode* pFbxChildNode = p_fbx_root_node->GetChild(i);

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


				FbxMesh* p_mesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

				// Deformer is basically a skeleton, so there will most likely only be one deformer per mesh
				unsigned int numOfDeformers = p_mesh->GetDeformerCount();
				for (unsigned int deformerIndex = 0; deformerIndex != numOfDeformers; ++deformerIndex)
				{
					FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(p_mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
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

				FbxVector4* p_vertices = p_mesh->GetControlPoints();

				// Populate the index vector
				int* p_indices = p_mesh->GetPolygonVertices();
				int index_count = p_mesh->GetPolygonVertexCount();
				pOutIndexVector->insert(pOutIndexVector->end(), &p_indices[0], &p_indices[index_count]);

				for (int j = 0; j < p_mesh->GetPolygonCount(); j++)
				{

					// Make sure the polygon is a triangle
					int num_vertices = p_mesh->GetPolygonSize(j);
					assert(num_vertices == 3);

					for (int k = 0; k < num_vertices; k++) {
						int control_point_index = p_mesh->GetPolygonVertex(j, k);

						FbxVertex vertex;
						vertex.pos[0] = (float)p_vertices[control_point_index].mData[0];
						vertex.pos[1] = (float)p_vertices[control_point_index].mData[1];
						vertex.pos[2] = (float)p_vertices[control_point_index].mData[2];

						fbxsdk::FbxVector4 nor;
						bool hasNor = p_mesh->GetPolygonVertexNormal(j, k, nor);
						// Check if the polygon has vertex normals
						// if so then save in vertex
						if (hasNor)
						{
							vertex.nor[0] = (float)nor.mData[0];
							vertex.nor[1] = (float)nor.mData[1];
							vertex.nor[2] = (float)nor.mData[2];
						}

						// Extract UVs
						int UVSetCount = p_mesh->GetUVLayerCount();
						if (UVSetCount > 0)
						{
							fbxsdk::FbxStringList UVSetNames;
							p_mesh->GetUVSetNames(UVSetNames);
							fbxsdk::FbxVector2 UV;
							bool unmapped;
							p_mesh->GetPolygonVertexUV(j, k, UVSetNames[0], UV, unmapped);
							vertex.uv[0] = (float)UV.mData[0];
							vertex.uv[1] = (float)UV.mData[1];
						}


						//bool hasUV = p_mesh->GetPolygonVertexUV(j, k, )

						pOutVertexVector->push_back(vertex);
					}
				}
			}
		}

	}
	return S_OK;
}
*/ 
