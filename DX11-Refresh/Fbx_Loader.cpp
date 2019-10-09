#include "Fbx_Loader.h"

// Anonymous namespace for Fbx_Loader
namespace {
	static FbxManager* gpFbxSdkManager = nullptr;

	DirectX::XMFLOAT4X4 ConvertFbxAMatrixToXMFLOAT4X4(FbxAMatrix &toConvert)
	{
		DirectX::XMFLOAT4X4 newMat;
		// Convert FbxMatrix to XMFLOAT
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				newMat.m[i][j] = static_cast<float>(toConvert.Get(i, j));
			}
		}
		return newMat;
	}

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
			if (skeleton->joints[i].mName == inJointName)
			{
				return i;
			}
		}

		throw std::exception("Skeleton information in FBX file is corrupted or invalid.");
	}
	// Traverse the tree of nodes depth-first and return the first mesh found
	FbxMesh* FindMesh(FbxNode* currentNode)
	{
		FbxMesh* mesh = nullptr;
		if (mesh = currentNode->GetMesh())
		{
			return mesh;
		}
		else
		{
			for (int i = 0; i < currentNode->GetChildCount(); ++i)
			{
				FbxNode* child = currentNode->GetChild(i);
				if (mesh = FindMesh(child))
				{
					return mesh;
				}
			}
			return nullptr;
		}
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
			curr_joint.mParentIndex = inParentIndex;
			curr_joint.mName = inNode->GetName();
			inSkeleton->joints.push_back(curr_joint);
		}
		for (int i = 0; i < inNode->GetChildCount(); ++i)
		{
			ProcessSkeletonHierarchyRecursively(inNode->GetChild(i), inSkeleton->joints.size(), myIndex, inSkeleton);
		}
	}

	// Traverse the Fbx and populate the Skeleton struct with the joints inside
	void ProcessSkeletonHierarchy(FbxNode* inRootNode, FbxLoader::Skeleton* inSkeleton)
	{
		for (int child_index = 0; child_index < inRootNode->GetChildCount(); ++child_index)
		{
			FbxNode* curr_node = inRootNode->GetChild(child_index);
			ProcessSkeletonHierarchyRecursively(curr_node, 0, -1, inSkeleton);
		}
	}

	void CheckSumOfWeights(std::vector<FbxLoader::ControlPointInfo>* jointData) {
		for (auto it = jointData->begin(); it != jointData->end(); ++it)
		{
			double sum_of_weights = 0.0;
			for (unsigned int i = 0; i < 4; ++i)
			{
				sum_of_weights += it->weightPairs[i].weight;
			}
			if ((sum_of_weights - 1.0) > 0.0005 || (sum_of_weights - 1.0) < -0.0005)
			{
				throw std::exception("Vertex weights do not add up to 1, please normalize the vertex weights.");
			}
		}
	}

	void ProcessJointsAndAnimations(FbxNode* inNode, FbxLoader::Skeleton* skeleton, std::vector<FbxLoader::ControlPointInfo>* jointData )
	{
		FbxMesh* curr_mesh = inNode->GetMesh();
		if (curr_mesh)
		{
			unsigned int num_deformers = curr_mesh->GetDeformerCount();
			// geometry transform is potentially something included with the model
			// not all modeling programs provide this
			// including it for safety because I don't know if blender does or not
			FbxAMatrix geometry_transform = GetGeometryTransformation(inNode);
			std::vector<std::vector<FbxLoader::IndexWeightPair>> temp(jointData->size(), std::vector<FbxLoader::IndexWeightPair>());
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
				FbxNode* boneRootNode = curr_skin->GetCluster(0)->GetLink();
				skeleton->joints[0].mBoneGlobalTransform = boneRootNode->EvaluateGlobalTransform();
				for (unsigned int cluster_index = 0; cluster_index < num_of_clusters; ++cluster_index)
				{
					FbxCluster* curr_cluster = curr_skin->GetCluster(cluster_index);

					std::string curr_joint_name = curr_cluster->GetLink()->GetName();
					unsigned int curr_joint_index = FindJointIndexUsingName(curr_joint_name, skeleton);
					FbxLoader::Joint* curr_joint = &skeleton->joints[curr_joint_index];
					FbxAMatrix transform_matrix;
					FbxAMatrix bind_pose_matrix;
					FbxAMatrix global_bindpose_inverse_matrix;

					curr_cluster->GetTransformMatrix(transform_matrix); // Bind pose mesh transform
					curr_cluster->GetTransformLinkMatrix(bind_pose_matrix); // Transformation of the joint at bind time from joint space to model space
					global_bindpose_inverse_matrix = bind_pose_matrix.Inverse() * transform_matrix;

					
					// Update skeleton
					skeleton->joints[curr_joint_index].mGlobalBindposeInverse = global_bindpose_inverse_matrix;
					skeleton->joints[curr_joint_index].mNode = curr_cluster->GetLink();

					// Get index weight pairs - https://www.gamedev.net/articles/programming/graphics/how-to-work-with-fbx-sdk-r3582
					// https://github.com/Larry955/FbxParser/blob/master/FbxParser/FbxParser.cpp - Rad 781 och 993

					// Fbx has every joint store the vertices it affects, we need to reverse this relationship
					unsigned int num_of_indices = curr_cluster->GetControlPointIndicesCount();					
					for (unsigned int i = 0; i < num_of_indices; ++i)
					{
						FbxLoader::IndexWeightPair curr_index_weight_pair;
						curr_index_weight_pair.index = curr_joint_index;
						curr_index_weight_pair.weight = curr_cluster->GetControlPointWeights()[i];
						temp[curr_cluster->GetControlPointIndices()[i]].push_back(curr_index_weight_pair);
					}

					// Get animation information
					FbxLongLong animation_length;
					std::string animation_name;

					FbxScene* p_scene = inNode->GetScene();
					FbxAnimStack* curr_anim_stack = FbxCast<FbxAnimStack>(inNode->GetScene()->GetSrcObject<FbxAnimStack>());
					// If file has animation
					if (curr_anim_stack)
					{
						FbxString anim_stack_name = curr_anim_stack->GetName();
						animation_name = anim_stack_name.Buffer();
						FbxTakeInfo* take_info = p_scene->GetTakeInfo(anim_stack_name);
						FbxTime start = curr_anim_stack->GetLocalTimeSpan().GetStart();
						FbxTime end = curr_anim_stack->GetLocalTimeSpan().GetStop();
						animation_length = end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1;


						FbxLoader::KeyFrame** curr_anim = &skeleton->joints[curr_joint_index].mAnimation;
						
						FbxDouble3 rot = curr_cluster->GetLink()->LclRotation.EvaluateValue(0.0f);
						FbxDouble3 transl = curr_cluster->GetLink()->LclTranslation.EvaluateValue(0.0f);
						curr_joint->mBoneLocalTransform = FbxAMatrix(transl, rot, FbxVector4(1.0f, 1.0f, 1.0f));
						if (curr_joint_index == 0)
						{
							curr_joint->mBoneGlobalTransform = curr_joint->mBoneLocalTransform;
						}
						else
						{
							curr_joint->mBoneGlobalTransform = skeleton->joints[skeleton->joints[curr_joint_index].mParentIndex].mBoneGlobalTransform * curr_joint->mBoneLocalTransform;

						}
						curr_joint->mGlobalBindposeInverse = curr_joint->mBoneGlobalTransform.Inverse();
						//curr_joint->mGlobalBindposeInverse = FbxAMatrix();
						//curr_joint->mGlobalBindposeInverse = FbxAMatrix(FbxVector4(0.0f, 0.0f, 0.0f, 1.0f), curr_joint->mBoneGlobalTransform.GetR(), curr_joint->mBoneGlobalTransform.GetS()).Inverse();
						//curr_joint->mOffsetMatrix = curr_joint->mGlobalBindposeInverse * curr_joint->mBoneGlobalTransform;

						unsigned int loopCounter = 0;
						curr_joint->mAnimationVector.reserve(animation_length);
						for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); ++i)
						{
							FbxTime curr_time;
							curr_time.SetFrame(i, FbxTime::eFrames24);
							*curr_anim = new FbxLoader::KeyFrame();
							(*curr_anim)->mFrameNum = i;

							FbxLoader::KeyFrame newSystemKeyFrame;
							newSystemKeyFrame.mFrameNum = i;

							rot = curr_cluster->GetLink()->LclRotation.EvaluateValue(curr_time);
							transl = curr_cluster->GetLink()->LclTranslation.EvaluateValue(curr_time);
							FbxAMatrix quatTest1 = FbxAMatrix(FbxVector4(0.0f, 0.0f, 0.0f, 0.0f), rot, FbxVector4(1.0f, 1.0f, 1.0f));
							FbxQuaternion quatTest2 = quatTest1.GetQ();
							newSystemKeyFrame.mLocalTransform = FbxAMatrix(transl, rot, FbxVector4(1.0f, 1.0f, 1.0f));

							if (curr_joint_index == 0)
							{
								newSystemKeyFrame.mGlobalTransform = newSystemKeyFrame.mLocalTransform;
							}
							else
							{
								newSystemKeyFrame.mGlobalTransform = skeleton->joints[skeleton->joints[curr_joint_index].mParentIndex].mAnimationVector[loopCounter].mGlobalTransform * newSystemKeyFrame.mLocalTransform;
								//newSystemKeyFrame.mGlobalTransform = newSystemKeyFrame.mLocalTransform * skeleton->joints[skeleton->joints[curr_joint_index].mParentIndex].mAnimationVector[loopCounter].mGlobalTransform;
								
							}
							
							newSystemKeyFrame.mOffsetMatrix = (curr_joint->mGlobalBindposeInverse * newSystemKeyFrame.mGlobalTransform);
							newSystemKeyFrame.mOffsetMatrix = (newSystemKeyFrame.mGlobalTransform * curr_joint->mGlobalBindposeInverse);


							DirectX::XMFLOAT4X4 bindPoseInverseXM = ConvertFbxAMatrixToXMFLOAT4X4(curr_joint->mGlobalBindposeInverse);
							DirectX::XMFLOAT4X4 keyframeGlobalTransformXM = ConvertFbxAMatrixToXMFLOAT4X4(newSystemKeyFrame.mGlobalTransform);
							DirectX::XMMATRIX bindPoseInverseXMMatrix = DirectX::XMLoadFloat4x4(&bindPoseInverseXM);
							DirectX::XMMATRIX keyframeGlobalTransformXMMatrix = DirectX::XMLoadFloat4x4(&keyframeGlobalTransformXM);
							DirectX::XMMATRIX result = DirectX::XMMatrixMultiply(bindPoseInverseXMMatrix, keyframeGlobalTransformXMMatrix);
							FbxVector4 offsetTranslation = newSystemKeyFrame.mOffsetMatrix.GetT();
							// Convert to correct coordinate system (maybe?)
							//newSystemKeyFrame.mOffsetMatrix = FbxAMatrix(FbxVector4(-offsetTranslation[1], offsetTranslation[0], -offsetTranslation[2], offsetTranslation[3]), newSystemKeyFrame.mOffsetMatrix.GetR(), newSystemKeyFrame.mOffsetMatrix.GetS());
							newSystemKeyFrame.mOffsetMatrix = newSystemKeyFrame.mOffsetMatrix.Transpose();
							curr_joint->mAnimationVector.push_back(newSystemKeyFrame);
							loopCounter++;
						}
					
					}
					else // Initialize Keyframes with identity matrices for non-animated meshes
					{
						FbxLoader::KeyFrame** curr_anim = &skeleton->joints[curr_joint_index].mAnimation;
						*curr_anim = new FbxLoader::KeyFrame();
					}


				}
			}
			for (unsigned int i = 0; i < temp.size(); ++i)
			{
				for (unsigned int j = 0; j < temp[i].size(); ++j)
				{
					(*jointData)[i].weightPairs[j] = temp[i][j];
					if (j >= MAX_NUM_WEIGHTS_PER_VERTEX)
					{
						throw std::exception("Mesh contains vertices with more than 4 bone weights. The engine does not support this.");
					}
				}
			}
			CheckSumOfWeights(jointData);
		}
	}


}



HRESULT FbxLoader::LoadFBX(const std::string& fileName, std::vector<DirectX::XMFLOAT3>* pOutVertexPosVector, std::vector<int>* pOutIndexVector,
	std::vector<DirectX::XMFLOAT3>* pOutNormalVector, std::vector<DirectX::XMFLOAT2>* pOutUVVector, Skeleton* pOutSkeleton, std::vector<ControlPointInfo>* pOutCPInfoVector)
{
	if (!pOutVertexPosVector || !pOutIndexVector || !pOutNormalVector || !pOutUVVector || !pOutSkeleton || !pOutCPInfoVector)
	{
		throw std::exception("One or more input vectors to LoadFBX were nullptr.");
	}
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

	//FbxAxisSystem direct_x_axis_system(FbxAxisSystem::eDirectX);
	//if (p_fbx_scene.get()->GetGlobalSettings().GetAxisSystem() != direct_x_axis_system)
	//{
	//	direct_x_axis_system.ConvertScene(p_fbx_scene.get());
	//}
	// Handle failed import
	if (!b_success)
	{
		FbxString error = p_importer->GetStatus().GetErrorString();
		OutputDebugStringA("error: Call to FbxImporter::Initialize() failed.\n");

		char buffer[100];
		sprintf_s(buffer, "error: Error returned: %s\n", error.Buffer());
		OutputDebugStringA(buffer);
		// Error checking, print to visual studio debug window
		if (p_importer->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion) {
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
	

	//FbxAxisSystem scene_axis_system = p_fbx_scene->GetGlobalSettings().GetAxisSystem();
	//FbxAxisSystem our_axis_system = FbxAxisSystem::eDirectX;
	//if (scene_axis_system != our_axis_system)
	//{
	//	our_axis_system.ConvertScene(p_fbx_scene.get());
	//}

	// Useful for skeleton/bone structure
	//DisplayHierarchy(p_fbx_scene);

	if (p_fbx_root_node)
	{
		::ProcessSkeletonHierarchy(p_fbx_root_node, pOutSkeleton);

			FbxNode* root_child = p_fbx_scene->GetRootNode();
			FbxMesh* p_mesh = ::FindMesh(p_fbx_root_node);
			if (p_mesh)
			{

				// Make sure the mesh is triangulated
				if (!p_mesh->IsTriangleMesh())
				{
					throw std::exception("Mesh contains non-triangles, please triangulate the mesh.");
				}

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


					// Save to control point info map 
					//control_points_info[j].ctrlPoint = p_mesh->GetControlPointAt(j);

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

				if (pOutSkeleton->joints.size() > 0)
				{
					// In progress, very likely to break
					// Prepare the vector with ControlPointInfo objects to be written to in any order
					pOutCPInfoVector->resize(p_mesh->GetControlPointsCount());
					::ProcessJointsAndAnimations(p_mesh->GetNode(), pOutSkeleton, pOutCPInfoVector);
				}

			}
			else
			{
				throw std::exception("No mesh found in .fbx file");
			}
	}
	return S_OK;
}