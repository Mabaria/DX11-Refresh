#include <fbxsdk.h>
#include <vector>
#include <d3d11.h>
#include <cassert>

struct MyVertex
{
	float pos[3];
	float nor[3];
	float uv[2];
};

namespace FbxLoader
{


	FbxManager* g_pFbxSdkManager = nullptr;

	HRESULT LoadFBX(std::vector<MyVertex>* pOutVertexVector, std::vector<int>* pOutIndexVector)
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
		bool bSuccess = pImporter->Initialize("C:\\Users\\magno\\Documents\\test1Triangulated.fbx", -1, g_pFbxSdkManager->GetIOSettings());
		if (!bSuccess) return E_FAIL;

		bSuccess = pImporter->Import(pFbxScene);
		if (!bSuccess) return E_FAIL;

		pImporter->Destroy();
		

		FbxNode* pFbxRootNode = pFbxScene->GetRootNode();

		if (pFbxRootNode)
		{
			for (int i = 0; i < pFbxRootNode->GetChildCount(); i++)
			{
				FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

				if (pFbxChildNode->GetNodeAttribute() == NULL)
					continue;
				std::string nodeName = pFbxChildNode->GetName();
				FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();


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

							MyVertex vertex;
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

}