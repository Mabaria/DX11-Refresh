#include "Fbx_Loader.h"

// Anonymous namespace for Fbx_Loader
namespace {
	static FbxManager* gpFbxSdkManager = nullptr;
	static FbxLoader::Skeleton skeleton;

	FbxAMatrix GetGeometryTransformation(FbxNode* node)
	{
		if (!node) {
			throw std::exception("Null for mesh geometry\n\n");
		}

		const FbxVector4 IT = node->GetGeometricTranslation(FbxNode::eSourcePivot);
		const FbxVector4 IR = node->GetGeometricRotation(FbxNode::eSourcePivot);
		const FbxVector4 IS = node->GetGeometricScaling(FbxNode::eSourcePivot);

		return FbxAMatrix(IT, IR, IS);

	}

	unsigned int FindJointIndexUsingName(const std::string& inJointName, FbxLoader::Skeleton* skeleton)
	{
		for (unsigned int i = 0; i < skeleton->joints.size(); ++i)
		{
			if (skeleton->joints[i].jointName.Compare(inJointName.c_str()) == 0)
			{
				return i;
			}
		}

		throw std::exception("Skeleton information in FBX file is corrupted.");
	}

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

	void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int myIndex, int inParentIndex, FbxLoader::Skeleton* inSkeleton)
	{
		if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() 
			&& inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			FbxLoader::Joint curr_joint;
			curr_joint.parentIndex = inParentIndex;
			curr_joint.currentIndex = myIndex;
			curr_joint.jointName = inNode->GetName();
			inSkeleton->joints.push_back(curr_joint);
		}
		for (int i = 0; i < inNode->GetChildCount(); ++i)
		{
			ProcessSkeletonHierarchyRecursively(inNode->GetChild(i), inSkeleton->joints.size(), myIndex, inSkeleton);
		}
	}

	void ProcessSkeletonHierarchy(FbxNode* inRootNode, FbxLoader::Skeleton* inSkeleton)
	{
		for (int child_index = 0; child_index < inRootNode->GetChildCount(); ++child_index)
		{
			FbxNode* curr_node = inRootNode->GetChild(child_index);
			ProcessSkeletonHierarchyRecursively(curr_node, 0, -1, inSkeleton);
		}
	}

	void ProcessJointsAndAnimations(FbxNode* inNode, FbxLoader::Skeleton* skeleton)
	{
		FbxMesh* curr_mesh = inNode->GetMesh();
		if (curr_mesh)
		{
			unsigned int num_deformers = curr_mesh->GetDeformerCount();
			// geometry transform is potentially something included with the model
			// not all modeling programs provide this
			// including it for safety because I don't know if blender does or not
			FbxAMatrix geometry_transform = GetGeometryTransformation(inNode);
			// Loop through deformers
			// Deformer is basically a skeleton
			// Most likely only on exists in the mesh
			for (unsigned int deformer_index = 0; deformer_index < num_deformers; ++deformer_index)
			{
				FbxSkin* curr_skin = static_cast<FbxSkin*>(curr_mesh->GetDeformer(deformer_index, FbxDeformer::eSkin));
				if (!curr_skin)
				{
					continue;
				}
				// Clusters contain links, which are basically joints
				unsigned int num_of_clusters = curr_skin->GetClusterCount();
				for (unsigned int cluster_index = 0; cluster_index < num_of_clusters; ++cluster_index)
				{
					FbxCluster* curr_cluster = curr_skin->GetCluster(cluster_index);
					std::string curr_joint_name = curr_cluster->GetLink()->GetName();
					unsigned int curr_joint_index = FindJointIndexUsingName(curr_joint_name, skeleton);
					FbxAMatrix transform_matrix;
					FbxAMatrix transform_link_matrix;
					FbxAMatrix global_bindpose_inverse_matrix;

					curr_cluster->GetTransformMatrix(transform_matrix); // Bind pose mesh transform
					curr_cluster->GetTransformLinkMatrix(transform_link_matrix); // Transformation of the joint at bind time from joint space to world space
					global_bindpose_inverse_matrix = transform_link_matrix.Inverse() * transform_matrix * geometry_transform;

					// Update skeleton
					skeleton->joints[curr_joint_index].mGlobalBindposeInverse = global_bindpose_inverse_matrix;
					skeleton->joints[curr_joint_index].mNode = curr_cluster->GetLink();
				}
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
	// Create Importer
	FbxImporter* p_importer = FbxImporter::Create(gpFbxSdkManager, "");

	// Create scene
	auto scene_deleter = [](FbxScene* scene) {scene->Destroy(); };
	std::unique_ptr<FbxScene, decltype(scene_deleter)> p_fbx_scene(FbxScene::Create(gpFbxSdkManager, ""), scene_deleter);

	// Import model
	bool b_success = p_importer->Initialize(fileName.c_str(), -1, gpFbxSdkManager->GetIOSettings());

	FbxAxisSystem direct_x_axis_system(FbxAxisSystem::eDirectX);
	if (p_fbx_scene.get()->GetGlobalSettings().GetAxisSystem() != direct_x_axis_system)
	{
		direct_x_axis_system.ConvertScene(p_fbx_scene.get());
	}
	// Handle failed import
	if (!b_success)
	{
		FbxString error = p_importer->GetStatus().GetErrorString();
		OutputDebugStringA("error: Call to FbxImporter::Initialize() failed.\n");

		char buffer[100];
		sprintf_s(buffer, "error: Error returned: %s\n", error.Buffer());
		OutputDebugStringA(buffer);
		// Error checking, print to visual studio debug window
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

	b_success = p_importer->Import(p_fbx_scene.get());
	if (!b_success) return E_FAIL;

	// Importer is no longer needed, remove from memory
	p_importer->Destroy();


	FbxNode* p_fbx_root_node = p_fbx_scene->GetRootNode();


	//// Useful for skeleton/bone structure
	//DisplayHierarchy(p_fbx_scene);

	if (p_fbx_root_node)
	{
		Skeleton testSkeleton;
		::ProcessSkeletonHierarchy(p_fbx_root_node, &testSkeleton);

		// Traverse the FBX tree
		for (int i = 0; i < p_fbx_root_node->GetChildCount(); i++)
		{
			FbxNode* p_fbx_child_node = p_fbx_root_node->GetChild(i);

			// If node has no attribute - not interested
			if (p_fbx_child_node->GetNodeAttribute() == NULL)
				continue;

			std::string node_name = p_fbx_child_node->GetName();
			FbxNodeAttribute::EType attribute_type = p_fbx_child_node->GetNodeAttribute()->GetAttributeType();

			std::unordered_map<int, ControlPointInfo> controlPointsInfo;

			// ---------- SKELETON RELATED CODE -----------------
			// Handle Skeleton
			// Skeletons in the FBX are stored with a null type root node
			// Go one step deeper to reach the skeleton
			/*
			if (attribute_type == FbxNodeAttribute::eNull)
			{
				p_fbx_child_node = p_fbx_child_node->GetChild(0);
				attribute_type = p_fbx_child_node->GetNodeAttribute()->GetAttributeType();
				FbxSkeleton* pSkeleton = (FbxSkeleton*)p_fbx_child_node->GetNodeAttribute();
				int num_children = p_fbx_child_node->GetChildCount();
			}
			*/
			// Handle Mesh attribute item
			if (attribute_type == FbxNodeAttribute::eMesh)
			{
				FbxMesh* p_mesh = (FbxMesh*)p_fbx_child_node->GetNodeAttribute();
				::ProcessJointsAndAnimations(p_fbx_child_node, &testSkeleton);

				// ---------- SKELETON RELATED CODE -----------------
				// ---------- SKELETON RELATED CODE -----------------
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

				// Make sure the mesh is triangulated
				assert(p_mesh->IsTriangleMesh() && "Mesh contains non-triangles, please triangulate the mesh.");

				FbxVector4* p_vertices = p_mesh->GetControlPoints();

				// Populate the returned index vector
				int* p_indices = p_mesh->GetPolygonVertices();
				int index_count = p_mesh->GetPolygonVertexCount();
				pOutIndexVector->insert(pOutIndexVector->end(), &p_indices[0], &p_indices[index_count]);

				// Load UVs and populate the returned UV vector
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