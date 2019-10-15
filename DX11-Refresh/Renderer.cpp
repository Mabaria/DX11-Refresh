#include "Renderer.h"

float scale = 1.0f;
float rotation = -0.0f;
float lastscroll = 0.0f;
int currentAnimFrame = 0;



Renderer::Renderer()
{
	this->Init();
	this->CreateShadersAndInputLayout();
	this->CreateConstantBuffers();
	this->CreateVertexBuffers();
	this->CreateSamplerState();
	this->CreateCubeMap();
	this->CreateFloorTexture();
	this->CreateSphere(10, 10);

	//this->ObjLoaderTest();
}

Renderer::~Renderer()
{
	if (this->mCamera != nullptr)
		delete this->mCamera;

	SafeRelease(&this->mDevice);
	SafeRelease(&this->mDeviceContext);
	SafeRelease(&this->mCubeIndexBuffer);
	SafeRelease(&this->mCubeVertexBuffer);
	SafeRelease(&this->mCubeVertexShader);
	SafeRelease(&this->mCubePixelShader);
	SafeRelease(&this->mRenderTargetView);
	SafeRelease(&this->mDepthStencilBuffer);
	SafeRelease(&this->mDepthStencilView);
	SafeRelease(&this->mDefaultInputLayout);
	SafeRelease(&this->mRasterState);
	SafeRelease(&this->mSwapChain);
	SafeRelease(&this->mWVPBuffer);

	sphereIndexBuffer->Release();
	sphereVertBuffer->Release();

	SKYBOX_VS->Release();
	SKYBOX_PS->Release();

	smrv->Release();

	DSLessEqual->Release();
	RSCullNone->Release();
}

void Renderer::Frame()
{
	this->gameTimer.Tick();
	this->HandleInput();


	sphereWorld = XMMatrixIdentity();

	//Define sphereWorld's world space matrix
	XMMATRIX Scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
	//Make sure the sphere is always centered around camera
	XMMATRIX Translation = XMMatrixTranslationFromVector(this->mCamera->GetPosition());

	//Set sphereWorld's world space using the transformations
	sphereWorld = Scale * Translation;


	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	this->mDeviceContext->ClearDepthStencilView(
		this->mDepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);
	this->mDeviceContext->ClearRenderTargetView(this->mRenderTargetView, this->mClearColor);



	this->mDeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->mDeviceContext->OMSetRenderTargets(1, &this->mRenderTargetView, this->mDepthStencilView);
	this->mDeviceContext->IASetInputLayout(mDefaultInputLayout);
	// ----- Render sphere
	this->mDeviceContext->IASetIndexBuffer(this->sphereIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	this->mDeviceContext->IASetVertexBuffers(0, 1, &this->sphereVertBuffer, &stride, &offset);

	auto WVP = this->sphereWorld * this->mCamera->GetViewMatrix() * this->mCamera->GetProjectionMatrix();
	WVP = XMMatrixTranspose(WVP);
	VS_WVP_CONSTANT_BUFFER vsConstData;
	DirectX::XMStoreFloat4x4(&vsConstData.mWorldViewProj, WVP);
	this->mDeviceContext->UpdateSubresource(
		this->mWVPBuffer,
		0,
		NULL,
		&vsConstData,
		0,
		0
	);
	this->mDeviceContext->VSSetConstantBuffers(0, 1, &this->mWVPBuffer);
	this->mDeviceContext->PSSetShaderResources(0, 1, &smrv);
	this->mDeviceContext->PSSetSamplers(0, 1, &skyboxSamplerState);

	this->mDeviceContext->VSSetShader(SKYBOX_VS, 0, 0);
	this->mDeviceContext->PSSetShader(SKYBOX_PS, 0, 0);
	this->mDeviceContext->OMSetDepthStencilState(DSLessEqual, 0);
	this->mDeviceContext->RSSetState(RSCullNone);
	this->mDeviceContext->DrawIndexed(NumSphereFaces * 3, 0, 0);


	// ----- Render cube
	this->mDeviceContext->VSSetShader(this->mCubeVertexShader, NULL, 0);
	this->mDeviceContext->PSSetShader(this->mCubePixelShader, NULL, 0);
	this->mDeviceContext->RSSetState(this->mRasterState);

	this->mDeviceContext->IASetVertexBuffers(0, 1, &this->mCubeVertexBuffer, &stride, &offset);
	this->mDeviceContext->IASetIndexBuffer(this->mCubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	// Set the WVP buffer.
	this->updateWVP(this->gameTimer.DeltaTime());
	this->mDeviceContext->PSSetShaderResources(0, 1, &mCubeTexSRV);
	this->mDeviceContext->VSSetConstantBuffers(0, 1, &this->mWVPBuffer);


	this->mDeviceContext->DrawIndexed(36, 0, 0);

	

	this->mDeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// ------ Render tests ------ 
	
	WVP = XMMatrixScaling(scale, scale, scale) * XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f) * this->mCamera->GetViewMatrix() * this->mCamera->GetProjectionMatrix();
	WVP = XMMatrixTranspose(WVP);
	DirectX::XMStoreFloat4x4(&vsConstData.mWorldViewProj, WVP);
	this->mDeviceContext->UpdateSubresource(
		this->mWVPBuffer,
		0,
		NULL,
		&vsConstData,
		0,
		0
	);
	this->mDeviceContext->VSSetShader(this->mCubeVertexShader, NULL, 0);
	this->mDeviceContext->PSSetShader(this->mCubePixelShader, NULL, 0);
	this->mDeviceContext->RSSetState(this->mRasterState);
	this->mDeviceContext->VSSetConstantBuffers(0, 1, &this->mWVPBuffer);
	this->mDeviceContext->PSSetShaderResources(0, 1, &mCubeTexSRV);
	//this->mDeviceContext->RSSetState(RSCullNone);
	int iter = 0;
	for (auto a : testVertexBuffers)
	{
		this->mDeviceContext->IASetVertexBuffers(0, 1, &a, &stride, &offset);
		this->mDeviceContext->IASetIndexBuffer(testIndexBuffers[iter], DXGI_FORMAT_R32_UINT, 0);
		this->mDeviceContext->DrawIndexed(testIndexCount[iter], 0, 0);
		iter++;
	}

	// ------ Render Skinned Meshes ------ 

	if (rotation > 0.01f || rotation < -0.01f)
	{
		std::vector<XMFLOAT4X4> temp;
		int iter = 0;
		currentAnimFrame = (currentAnimFrame + 1) % this->skinSkeletons[0]->joints[0].mAnimationVector.size();
		for (auto j : this->skinSkeletons[0]->joints)
		{
			temp.push_back(j.mAnimationVector[currentAnimFrame].mOffsetMatrix);
			skinBoneMatrices[0][iter++] = j.mAnimationVector[currentAnimFrame].mOffsetMatrix;
		}
		VS_BONE_CONSTANT_BUFFER vsConstData = {};
		for (int i = 0; i < this->skinSkeletons[0]->joints.size() && i < MAX_NUMBER_OF_BONES_IN_SHADER; i++)
		{
			vsConstData.mBoneTransforms[i] = temp[i];
		}

		this->mDeviceContext->UpdateSubresource(
			this->mBoneTransformBuffer,
			0,
			NULL,
			&vsConstData,
			0,
			0
		);
		
		rotation = 0.0f;
	}
	this->mDeviceContext->IASetInputLayout(mSkinInputLayout);
	this->mDeviceContext->VSSetShader(this->mSkinVertexShader, NULL, 0);
	this->mDeviceContext->VSSetConstantBuffers(1, 1, &this->mBoneTransformBuffer);
	UINT skinStride = sizeof(SkinVertex);
	iter = 0;
	for (auto a : skinVertexBuffers)
	{
		VS_BONE_CONSTANT_BUFFER vsBoneData = {};
		for (int i = 0; i < skinBoneMatrices[0].size() && i < MAX_NUMBER_OF_BONES_IN_SHADER; i++)
		{
			vsBoneData.mBoneTransforms[i] = skinBoneMatrices[0][i];
		}
		//XMStoreFloat4x4(&vsBoneData.mBoneTransforms[9], XMMatrixRotationX(rotation));
		this->mDeviceContext->UpdateSubresource(
			this->mBoneTransformBuffer,
			0,
			NULL,
			&vsBoneData,
			0,
			0
		);
		this->mDeviceContext->IASetVertexBuffers(0, 1, &a, &skinStride, &offset);
		this->mDeviceContext->IASetIndexBuffer(skinIndexBuffers[iter], DXGI_FORMAT_R32_UINT, 0);
		this->mDeviceContext->DrawIndexed(skinIndexCount[iter], 0, 0);
		iter++;
	}


	HRESULT hr = this->mSwapChain->Present(0, 0);
}

void Renderer::MouseMoved(int x, int y)
{
	this->mCamera->RotateCameraPitchYawRoll(x / 100.0f, y / 100.0f, 0.0f);
}

void Renderer::KeyPressed(WPARAM key)
{
	int ascii = (int)key;
	switch (ascii)
	{
	case 87: // W
		this->mCamera->MoveCamera(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), 0.1f);
		break;
	case 83: // S
		this->mCamera->MoveCamera(DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), 0.1f);
		break;
	case 65: // A
		this->mCamera->MoveCamera(DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), 0.1f);
		break;
	case 68: // D
		this->mCamera->MoveCamera(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.1f);
		break;
	}
}

void Renderer::LoadMesh(std::string& filepath)
{
	this->objLoader.LoadFile(filepath);
	std::vector<objl::Mesh> meshes = objLoader.LoadedMeshes;


	for (auto a : meshes)
	{
		ID3D11Buffer* verBuf = nullptr;
		ID3D11Buffer* indBuf = nullptr;

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(objl::Vertex) * a.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &a.Vertices[0];

		HRESULT hr = this->mDevice->CreateBuffer(
			&vbd,
			&vinitData,
			&verBuf
		);


		D3D11_BUFFER_DESC ibd;
		ZeroMemory(&ibd, sizeof(D3D11_BUFFER_DESC));
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * a.Indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &a.Indices[0];

		hr = this->mDevice->CreateBuffer(&ibd, &iinitData, &indBuf);

		testVertexBuffers.push_back(verBuf);
		testIndexBuffers.push_back(indBuf);
		testIndexCount.push_back(a.Indices.size());
	}
}

void Renderer::LoadMesh(std::string& filepath, bool fbx)
{

	MeshObject mesh;
	mesh.LoadFBX(filepath);
	std::vector<DirectX::XMFLOAT3>* vertexPositions = mesh.GetVertexPositionVector();
	std::vector<int>* vertexIndices = mesh.GetIndexVector();
	std::vector<DirectX::XMFLOAT3>* normals = mesh.GetNormalVector();
	std::vector<DirectX::XMFLOAT2>* UVs = mesh.GetUVVector();
	FbxLoader::Skeleton* skeleton = mesh.GetSkeleton();
	std::vector<FbxLoader::ControlPointInfo>* skinWeights = mesh.GetSkinningWeights();
	ID3D11Buffer* verBuf = nullptr;
	ID3D11Buffer* indBuf = nullptr;
	if (vertexIndices)
	{
		// Non-skinned mesh
		if (!skeleton)
		{
			objl::Vertex* input_vertices = new objl::Vertex[vertexPositions->size()];
			for (int i = 0; i < vertexPositions->size(); ++i)
			{
				input_vertices[i].Position.X = (*vertexPositions)[i].x;
				input_vertices[i].Position.Y = (*vertexPositions)[i].y;
				input_vertices[i].Position.Z = (*vertexPositions)[i].z;
				if (mesh.HasUVs())
				{
					input_vertices[i].TextureCoordinate.X = (*UVs)[i].x;
					input_vertices[i].TextureCoordinate.Y = (*UVs)[i].y;
				}
				if (mesh.HasNormals())
				{
					input_vertices[i].Normal.X = (*normals)[i].x;
					input_vertices[i].Normal.Y = (*normals)[i].y;
					input_vertices[i].Normal.Z = (*normals)[i].z;
				}

			}

			D3D11_BUFFER_DESC vbd;
			vbd.Usage = D3D11_USAGE_IMMUTABLE;
			vbd.ByteWidth = sizeof(objl::Vertex) * vertexPositions->size();
			vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vbd.CPUAccessFlags = 0;
			vbd.MiscFlags = 0;
			vbd.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA vinitData;
			vinitData.pSysMem = input_vertices;

			HRESULT hr = this->mDevice->CreateBuffer(
				&vbd,
				&vinitData,
				&verBuf
			);


			D3D11_BUFFER_DESC ibd;
			ZeroMemory(&ibd, sizeof(D3D11_BUFFER_DESC));
			ibd.Usage = D3D11_USAGE_IMMUTABLE;
			ibd.ByteWidth = sizeof(UINT) * vertexIndices->size();
			ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			ibd.CPUAccessFlags = 0;
			ibd.MiscFlags = 0;
			ibd.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA iinitData;
			iinitData.pSysMem = vertexIndices->data();

			hr = this->mDevice->CreateBuffer(&ibd, &iinitData, &indBuf);

			testVertexBuffers.push_back(verBuf);
			testIndexBuffers.push_back(indBuf);
			testIndexCount.push_back(vertexIndices->size());
		}
		else
		{
			SkinVertex* input_vertices = new SkinVertex[vertexPositions->size()];
			for (int i = 0; i < vertexPositions->size(); ++i)
			{
				input_vertices[i].Pos.x = (*vertexPositions)[i].x;
				input_vertices[i].Pos.y = (*vertexPositions)[i].y;
				input_vertices[i].Pos.z = (*vertexPositions)[i].z;
				if (mesh.HasUVs())
				{
					input_vertices[i].Texcoord.x = (*UVs)[i].x;
					input_vertices[i].Texcoord.y = (*UVs)[i].y;
				}
				if (mesh.HasNormals())
				{
					input_vertices[i].Normal.x = (*normals)[i].x;
					input_vertices[i].Normal.y = (*normals)[i].y;
					input_vertices[i].Normal.z = (*normals)[i].z;
				}
				input_vertices[i].BlendWeights.x = (*skinWeights)[i].weightPairs[0].weight;
				input_vertices[i].BlendIndices.x = (*skinWeights)[i].weightPairs[0].index;
				input_vertices[i].BlendWeights.y = (*skinWeights)[i].weightPairs[1].weight;
				input_vertices[i].BlendIndices.y = (*skinWeights)[i].weightPairs[1].index;
				input_vertices[i].BlendWeights.z = (*skinWeights)[i].weightPairs[2].weight;
				input_vertices[i].BlendIndices.z = (*skinWeights)[i].weightPairs[2].index;
				input_vertices[i].BlendIndices.w = (*skinWeights)[i].weightPairs[3].index;
				// Last weight not needed as it can be calculated in the shader
			}

				D3D11_BUFFER_DESC vbd;
				vbd.Usage = D3D11_USAGE_IMMUTABLE;
				vbd.ByteWidth = sizeof(SkinVertex) * vertexPositions->size();
				vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				vbd.CPUAccessFlags = 0;
				vbd.MiscFlags = 0;
				vbd.StructureByteStride = 0;

				D3D11_SUBRESOURCE_DATA vinitData;
				vinitData.pSysMem = input_vertices;

				HRESULT hr = this->mDevice->CreateBuffer(
					&vbd,
					&vinitData,
					&verBuf
				);


				D3D11_BUFFER_DESC ibd;
				ZeroMemory(&ibd, sizeof(D3D11_BUFFER_DESC));
				ibd.Usage = D3D11_USAGE_IMMUTABLE;
				ibd.ByteWidth = sizeof(UINT) * vertexIndices->size();
				ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
				ibd.CPUAccessFlags = 0;
				ibd.MiscFlags = 0;
				ibd.StructureByteStride = 0;

				D3D11_SUBRESOURCE_DATA iinitData;
				iinitData.pSysMem = vertexIndices->data();

				hr = this->mDevice->CreateBuffer(&ibd, &iinitData, &indBuf);
				std::vector<XMFLOAT4X4> temp;
				FbxAMatrix tempFbxMatrix;
				int iter = 0;
				for (auto a : skeleton->joints)
				{
					// I dont know what the fuck im doing
					// tempFbxMatrix = a.mGlobalBindposeInverse.Inverse() * FbxAMatrix(FbxVector4(0.0f, 0.0f, 0.0f), FbxVector4(0.0f, 0.0f, 0.0f, 0.0f), FbxVector4(1.0f, 1.0f, 1.0f)) * a.mGlobalBindposeInverse;
					//tempFbxMatrix = a.mGlobalBindposeInverse * a.mAnimation->mGlobalTransform;
					//tempFbxMatrix = a.mAnimationVector[0].mOffsetMatrix;
					//tempFbxMatrix = a.mAnimation->mGlobalTransform;

					//currentAnimFrame = (currentAnimFrame + 1) % this->skinSkeletons[0]->joints[0].mAnimationVector.size();

					temp.push_back(a.mAnimationVector[0].mOffsetMatrix);
				}
				skinBoneMatrices.push_back(temp);
				VS_BONE_CONSTANT_BUFFER vsConstData = {};
				for (int i = 0; i < skeleton->joints.size() && i < MAX_NUMBER_OF_BONES_IN_SHADER; i++)
				{
					vsConstData.mBoneTransforms[i] = temp[i];
				}
				
				this->mDeviceContext->UpdateSubresource(
					this->mBoneTransformBuffer,
					0,
					NULL,
					&vsConstData,
					0,
					0
				);
				skinVertexBuffers.push_back(verBuf);
				skinIndexBuffers.push_back(indBuf);
				skinIndexCount.push_back(vertexIndices->size());
				skinSkeletons.push_back(skeleton);
		}
	}
}

bool Renderer::Init()
{
	HRESULT hr;


	HWND activeWindow = GetActiveWindow();
	RECT activeWindowRect;
	GetWindowRect(activeWindow, &activeWindowRect);

	this->mAspectRatio = (float)(activeWindowRect.right - activeWindowRect.left) / (float)(activeWindowRect.bottom - activeWindowRect.top);


	this->mCamera = new Camera(DirectX::XMVectorSet(0.0f, 5.0f, -5.0f, 1.0f),
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
		DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		70.0f,
		this->mAspectRatio,
		1.0f,
		1000.0f,
		LOOK_MODE::LOOK_TO
	);

	

	DXGI_MODE_DESC modeDesc;
	modeDesc.Width						= activeWindowRect.right - activeWindowRect.left;
	modeDesc.Height						= activeWindowRect.bottom - activeWindowRect.top;
	modeDesc.RefreshRate.Numerator		= 60; // Refresh rate set to 60
	modeDesc.RefreshRate.Denominator	= 1;
	modeDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	modeDesc.ScanlineOrdering			= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	modeDesc.Scaling					= DXGI_MODE_SCALING_UNSPECIFIED;

	//// checking for multisample support
	//UINT numQualityLevels				= 0;

	//mDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &numQualityLevels);

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferDesc			= modeDesc;
	swapChainDesc.SampleDesc.Count		= 1;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount			= 1;
	swapChainDesc.Windowed				= true;
	swapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags					= 0;
	swapChainDesc.OutputWindow			= activeWindow;

	// Get dxgiFactory for creation of swapchain 

	//IDXGIDevice* dxgiDevice = 0;
	//mDevice->QueryInterface(__uuidof(IDXGIDevice),
	//	(void**)& dxgiDevice);

	//IDXGIAdapter* dxgiAdapter = 0;
	//dxgiDevice->GetAdapter(&dxgiAdapter);

	//IDXGIFactory* dxgiFactory = 0;
	//dxgiAdapter->GetParent(__uuidof(IDXGIFactory),
	//	(void**)& dxgiFactory);

	//dxgiDevice->Release();
	//dxgiAdapter->Release();
	//dxgiFactory->Release();

	hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&this->mSwapChain,
		&this->mDevice,
		NULL,
		&this->mDeviceContext);
	
	//hr = dxgiFactory->CreateSwapChain(mDevice, &swapChainDesc, &mSwapChain);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateDeviceAndSwapChain failed.", 0, 0);
		return false;
	}


	hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&mBackBuffer));

	if (FAILED(hr))
	{
		MessageBox(0, L"GetBuffer failed.", 0, 0);
		return false;
	}
	if (mBackBuffer != nullptr)
	{
		hr = mDevice->CreateRenderTargetView(mBackBuffer, 0, &mRenderTargetView);

		if (FAILED(hr))
		{
			MessageBox(0, L"CreateRenderTargetView failed.", 0, 0);
			return false;
		}
	}
	else
	{
		MessageBox(0, L"backBuffer was nullptr.", 0, 0);
		return false;
	}
	
	mBackBuffer->Release();

	// Create depth stencil buffer and view

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));
	depthStencilDesc.Width					= activeWindowRect.right - activeWindowRect.left;
	depthStencilDesc.Height					= activeWindowRect.bottom - activeWindowRect.top;
	depthStencilDesc.MipLevels				= 1;
	depthStencilDesc.ArraySize				= 1;
	depthStencilDesc.Format					= DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 4x MSAA DISABLED CURRENTLY
	depthStencilDesc.SampleDesc.Count		= 1;
	//depthStencilDesc.SampleDesc.Quality		= numQualityLevels - 1;

	depthStencilDesc.Usage					= D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags				= D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags			= 0;
	depthStencilDesc.MiscFlags				= 0;

	hr = this->mDevice->CreateTexture2D(
		&depthStencilDesc,
		0,
		&this->mDepthStencilBuffer
	);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateTexture2D failed for depthStencilBuffer.", 0, 0);
		return false;
	}

	hr = this->mDevice->CreateDepthStencilView(
		this->mDepthStencilBuffer,
		0,
		&this->mDepthStencilView
	);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateDepthStencilView failed.", 0, 0);
		return false;
	}


	// Set render target
	this->mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

	// Create/Set viewport

	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width	= static_cast<float>(activeWindowRect.right - activeWindowRect.left);
	vp.Height	= static_cast<float>(activeWindowRect.bottom - activeWindowRect.top);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 0.0f;

	this->mDeviceContext->RSSetViewports(1, &vp);

	// Start game timer
	this->gameTimer.Start();

	// Initialize keyboard and mouse
	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<DirectX::Mouse>();
	m_mouse->SetWindow(activeWindow);

	return true;
}

bool Renderer::CreateVertexBuffers()
{
	Vertex vertices[] =
	{
	{ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f)},
	{ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f)},
	{ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 50.0f)},
	{ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f)},
	{ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f)},
	{ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f), DirectX::XMFLOAT2(50.0f, 0.0f)},
	{ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(50.0f, 50.0f)},
	{ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f)}
	};
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = vertices;

	HRESULT hr = this->mDevice->CreateBuffer(
		&vbd,
		&vinitData,
		&mCubeVertexBuffer
	);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateBuffer for cube vertex buffer failed", 0, 0);
		return false;
	}

	UINT indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(D3D11_BUFFER_DESC));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * 36;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = indices;

	hr = this->mDevice->CreateBuffer(&ibd, &iinitData, &this->mCubeIndexBuffer);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateBuffer for cube index buffer failed", 0, 0);
		return false;
	}

	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FrontCounterClockwise = false;
	rsDesc.DepthClipEnable = true;
	hr = mDevice->CreateRasterizerState(&rsDesc, &mRasterState);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateRasterizerState failed", 0, 0);
		return false;
	}

	return true;
}

bool Renderer::CreateShadersAndInputLayout()
{
	// Compile vertex shader
	ID3DBlob* vs_blob = nullptr;
	HRESULT hr;
	hr = D3DCompileFromFile(
		L"VertexShader.hlsl",
		nullptr,
		nullptr,
		"VS",
		"vs_5_0",
		0,
		0,
		&vs_blob,
		nullptr
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3DCompileFromFile Compiling Vertex Shader failed", 0, 0);
		return false;
	}
	// Create vertex shader
	hr = this->mDevice->CreateVertexShader(
		vs_blob->GetBufferPointer(),
		vs_blob->GetBufferSize(),
		nullptr,
		&mCubeVertexShader
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateVertexShader failed", 0, 0);
		return false;
	}



	// Compile skinning VS

	ID3DBlob* skin_vs_blob = nullptr;
	hr = D3DCompileFromFile(
		L"SkinningVS.hlsl",
		nullptr,
		nullptr,
		"SKIN_VS",
		"vs_5_0",
		0,
		0,
		&skin_vs_blob,
		nullptr
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3DCompileFromFile Compiling Skinning Vertex Shader failed", 0, 0);
		return false;
	}

	// Create skinning VS

	hr = this->mDevice->CreateVertexShader(
		skin_vs_blob->GetBufferPointer(),
		skin_vs_blob->GetBufferSize(),
		nullptr,
		&mSkinVertexShader
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateVertexShader failed for skin_vs", 0, 0);
		return false;
	}



	// Create input layout

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = mDevice->CreateInputLayout(
		vertexDesc,
		3,
		vs_blob->GetBufferPointer(),
		vs_blob->GetBufferSize(),
		&this->mDefaultInputLayout
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateInputLayout failed", 0, 0);
		return false;
	}
	// Set the input layout
	this->mDeviceContext->IASetInputLayout(this->mDefaultInputLayout);

	// Create input layout for skinning VS
	D3D11_INPUT_ELEMENT_DESC skinVertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = mDevice->CreateInputLayout(
		skinVertexDesc,
		5,
		skin_vs_blob->GetBufferPointer(),
		skin_vs_blob->GetBufferSize(),
		&this->mSkinInputLayout
	);


	// Compile pixel shader
	ID3DBlob* ps_blob = nullptr;
	hr = D3DCompileFromFile(
		L"PixelShader.hlsl",
		nullptr,
		nullptr,
		"PS",
		"ps_5_0",
		0,
		0,
		&ps_blob,
		nullptr
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3DCompileFromFile Compiling Pixel Shader failed", 0, 0);
		return false;
	}
	// Create Pixel shader
	hr = this->mDevice->CreatePixelShader(
		ps_blob->GetBufferPointer(),
		ps_blob->GetBufferSize(),
		nullptr,
		&mCubePixelShader
	);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreatePixelShader failed", 0, 0);
		return false;
	}

	skin_vs_blob->Release();
	vs_blob->Release();
	ps_blob->Release();

	//------ Skybox shaders below -----------------------------------
	// --------------------------------------------------------------

		// Compile skybox vertex shader
	ID3DBlob* skybox_vs_blob = nullptr;
	hr = D3DCompileFromFile(
		L"SkyboxVS.hlsl",
		nullptr,
		nullptr,
		"SKYBOX_VS",
		"vs_5_0",
		0,
		0,
		&skybox_vs_blob,
		nullptr
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3DCompileFromFile Compiling Skybox Vertex Shader failed", 0, 0);
		return false;
	}
	// Create vertex shader
	hr = this->mDevice->CreateVertexShader(
		skybox_vs_blob->GetBufferPointer(),
		skybox_vs_blob->GetBufferSize(),
		nullptr,
		&SKYBOX_VS
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateVertexShader for Skybox failed", 0, 0);
		return false;
	}

	skybox_vs_blob->Release();

	// Compile pixel shader
	ID3DBlob* skybox_ps_blob = nullptr;
	hr = D3DCompileFromFile(
		L"SkyboxPS.hlsl",
		nullptr,
		nullptr,
		"SKYBOX_PS",
		"ps_5_0",
		0,
		0,
		&skybox_ps_blob,
		nullptr
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3DCompileFromFile Compiling Pixel Shader for Skybox failed", 0, 0);
		return false;
	}
	// Create Pixel shader
	hr = this->mDevice->CreatePixelShader(
		skybox_ps_blob->GetBufferPointer(),
		skybox_ps_blob->GetBufferSize(),
		nullptr,
		&SKYBOX_PS
	);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreatePixelShader failed for Skybox", 0, 0);
		return false;
	}
	return true;
}

bool Renderer::CreateConstantBuffers()
{
	DirectX::XMStoreFloat4x4(&mCubeWorld, DirectX::XMMatrixIdentity());

	DirectX::XMMATRIX view = this->mCamera->GetViewMatrix();

	DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, this->mAspectRatio, 1.0f, 1000.0f);
	DirectX::XMStoreFloat4x4(&mProjection, proj);

	DirectX::XMMATRIX wvp = DirectX::XMMatrixMultiply(view, proj);

	VS_WVP_CONSTANT_BUFFER vsConstData;
	DirectX::XMStoreFloat4x4(&vsConstData.mWorldViewProj, wvp);
	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(VS_WVP_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &vsConstData;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Create the buffer.
	HRESULT hr = this->mDevice->CreateBuffer(&cbDesc, &InitData,
		&this->mWVPBuffer);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateBuffer for WVP failed", 0, 0);
		return false;
	}

	VS_BONE_CONSTANT_BUFFER vsSkeletonData;
	
	D3D11_BUFFER_DESC skincbDesc;
	skincbDesc.ByteWidth = sizeof(VS_BONE_CONSTANT_BUFFER);
	skincbDesc.Usage = D3D11_USAGE_DEFAULT;
	skincbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	skincbDesc.CPUAccessFlags = 0;
	skincbDesc.MiscFlags = 0;
	skincbDesc.StructureByteStride = 0;

	hr = this->mDevice->CreateBuffer(&skincbDesc, NULL, &this->mBoneTransformBuffer);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateBuffer for boneTransformBuffer failed", 0, 0);
		return false;
	}
	return true;
}

bool Renderer::CreateSamplerState()
{
	// Describe the Sample State
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	//Create the Sample State
	HRESULT hr = this->mDevice->CreateSamplerState(&sampDesc, &skyboxSamplerState);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateSamplerState for Skybox failed", 0, 0);
		return false;
	}
	return true;
}

bool Renderer::CreateCubeMap()
{
	HRESULT hr;
	ID3D11Texture2D* SMTexture = 0;

	hr = CreateDDSTextureFromFile(this->mDevice,
		this->mDeviceContext,
		L"Skybox3.dds",
		(ID3D11Resource * *)&SMTexture,
		&smrv);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateDDSTextureFromFile for Skybox failed", 0, 0);
		return false;
	}
	//D3D11_TEXTURE2D_DESC SMTextureDesc;
	//SMTexture->GetDesc(&SMTextureDesc);

	//D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	//SMViewDesc.Format = SMTextureDesc.Format;
	//SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	//SMViewDesc.TextureCube.MipLevels = SMTextureDesc.MipLevels;
	//SMViewDesc.TextureCube.MostDetailedMip = 0;

	// Create rasterizer state with culling off
	D3D11_RASTERIZER_DESC cmdesc;
	ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));
	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_NONE;
	cmdesc.FrontCounterClockwise = true;
	hr = this->mDevice->CreateRasterizerState(&cmdesc, &RSCullNone);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateRasterizerState for Skybox failed", 0, 0);
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC dssDesc;
	ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	hr = this->mDevice->CreateDepthStencilState(&dssDesc, &DSLessEqual);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateDepthStencilState for Skybox failed", 0, 0);
		return false;
	}

	return true;
}

bool Renderer::CreateFloorTexture()
{
	HRESULT hr;
	ID3D11Texture2D* SMTexture = 0;

	hr = CreateDDSTextureFromFile(this->mDevice,
		this->mDeviceContext,
		L"../Textures/UnitTexture.dds",
		(ID3D11Resource * *)& SMTexture,
		&mCubeTexSRV);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateDDSTextureFromFile for Floor failed", 0, 0);
		return false;
	}
	return true;
}

void Renderer::CreateSphere(int LatLines, int LongLines)
{
	NumSphereVertices = ((LatLines - 2) * LongLines) + 2;
	NumSphereFaces = ((LatLines - 3) * (LongLines) * 2) + (LongLines * 2);

	float sphereYaw = 0.0f;
	float spherePitch = 0.0f;

	std::vector<Vertex> vertices(NumSphereVertices);

	DirectX::XMVECTOR currVertPos = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	vertices[0].Pos.x = 0.0f;
	vertices[0].Pos.y = 0.0f;
	vertices[0].Pos.z = 1.0f;

	for (DWORD i = 0; i < LatLines - 2; ++i)
	{
		spherePitch = (i + 1) * (3.14 / (LatLines - 1));
		Rotationx = XMMatrixRotationX(spherePitch);
		for (DWORD j = 0; j < LongLines; ++j)
		{
			sphereYaw = j * (6.28 / (LongLines));
			Rotationy = XMMatrixRotationZ(sphereYaw);
			currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (Rotationx * Rotationy));
			currVertPos = XMVector3Normalize(currVertPos);
			vertices[i * LongLines + j + 1].Pos.x = XMVectorGetX(currVertPos);
			vertices[i * LongLines + j + 1].Pos.y = XMVectorGetY(currVertPos);
			vertices[i * LongLines + j + 1].Pos.z = XMVectorGetZ(currVertPos);

		}

		vertices[NumSphereVertices - 1].Pos.x = 0.0f;
		vertices[NumSphereVertices - 1].Pos.y = 0.0f;
		vertices[NumSphereVertices - 1].Pos.z = -1.0f;


		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(Vertex) * NumSphereVertices;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA vertexBufferData;

		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
		vertexBufferData.pSysMem = &vertices[0];
		HRESULT hr = this->mDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &sphereVertBuffer);


		std::vector<DWORD> indices(NumSphereFaces * 3);

		int k = 0;
		for (DWORD l = 0; l < LongLines - 1; ++l)
		{
			indices[k] = 0;
			indices[k + 1] = l + 1;
			indices[k + 2] = l + 2;
			k += 3;
		}

		indices[k] = 0;
		indices[k + 1] = LongLines;
		indices[k + 2] = 1;
		k += 3;

		for (DWORD i = 0; i < LatLines - 3; ++i)
		{
			for (DWORD j = 0; j < LongLines - 1; ++j)
			{
				indices[k] = i * LongLines + j + 1;
				indices[k + 1] = i * LongLines + j + 2;
				indices[k + 2] = (i + 1) * LongLines + j + 1;

				indices[k + 3] = (i + 1) * LongLines + j + 1;
				indices[k + 4] = i * LongLines + j + 2;
				indices[k + 5] = (i + 1) * LongLines + j + 2;

				k += 6; // next quad
			}

			indices[k] = (i * LongLines) + LongLines;
			indices[k + 1] = (i * LongLines) + 1;
			indices[k + 2] = ((i + 1) * LongLines) + LongLines;

			indices[k + 3] = ((i + 1) * LongLines) + LongLines;
			indices[k + 4] = (i * LongLines) + 1;
			indices[k + 5] = ((i + 1) * LongLines) + 1;

			k += 6;
		}

		for (DWORD l = 0; l < LongLines - 1; ++l)
		{
			indices[k] = NumSphereVertices - 1;
			indices[k + 1] = (NumSphereVertices - 1) - (l + 1);
			indices[k + 2] = (NumSphereVertices - 1) - (l + 2);
			k += 3;
		}

		indices[k] = NumSphereVertices - 1;
		indices[k + 1] = (NumSphereVertices - 1) - LongLines;
		indices[k + 2] = NumSphereVertices - 2;

		D3D11_BUFFER_DESC indexBufferDesc;
		ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(DWORD) * NumSphereFaces * 3;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA iinitData;

		iinitData.pSysMem = &indices[0];
		this->mDevice->CreateBuffer(&indexBufferDesc, &iinitData, &sphereIndexBuffer);
	}
}

void Renderer::ObjLoaderTest()
{
	this->objLoader.LoadFile("../Models/BirchTree_2.obj");
	std::vector<objl::Mesh> meshes = objLoader.LoadedMeshes;


	for (auto a : meshes)
	{
		ID3D11Buffer* verBuf = nullptr;
		ID3D11Buffer* indBuf = nullptr;

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(objl::Vertex) * a.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &a.Vertices[0];

		HRESULT hr = this->mDevice->CreateBuffer(
			&vbd,
			&vinitData,
			&verBuf
		);


		D3D11_BUFFER_DESC ibd;
		ZeroMemory(&ibd, sizeof(D3D11_BUFFER_DESC));
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * a.Indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &a.Indices[0];

		hr = this->mDevice->CreateBuffer(&ibd, &iinitData, &indBuf);

		testVertexBuffers.push_back(verBuf);
		testIndexBuffers.push_back(indBuf);
		testIndexCount.push_back(a.Indices.size());
	}
}

void Renderer::updateWVP(float dt)
{
	DirectX::XMMATRIX newWorld = DirectX::XMLoadFloat4x4(&this->mCubeWorld);
	//newWorld *= DirectX::XMMatrixRotationRollPitchYaw(dt, dt*0.1f, 0.0f);

	DirectX::XMStoreFloat4x4(&this->mCubeWorld, newWorld);
	DirectX::XMMATRIX view = this->mCamera->GetViewMatrix();

	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&this->mProjection);
	// Make cube into floor
	newWorld *= XMMatrixScaling(50.0f, 0.1f, 50.0f);
	DirectX::XMMATRIX wvp = XMMatrixTranspose(DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(newWorld, view), proj));

	VS_WVP_CONSTANT_BUFFER vsConstData;
	DirectX::XMStoreFloat4x4(&vsConstData.mWorldViewProj, wvp);

	this->mDeviceContext->UpdateSubresource(
		this->mWVPBuffer,
		0,
		NULL,
		&vsConstData,
		0,
		0
	);

}

void Renderer::HandleInput()
{
	auto kb = this->m_keyboard->GetState();
	auto mouse = this->m_mouse->GetState();
	float dt = this->gameTimer.DeltaTime();
	float speed = 10.0f * dt;
	if (kb.LeftShift)
		speed *= 2;
	XMVECTOR finalMovement = XMVectorZero();
	if (kb.W)
	{
		//this->mCamera->MoveCamera(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), speed);
		//this->mCamera->MoveCamera(this->mCamera->GetLookVector(), speed);
		finalMovement += this->mCamera->GetLookVector();
	}
	if (kb.S)
	{
		//this->mCamera->MoveCamera(DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), speed);
		//this->mCamera->MoveCamera(-this->mCamera->GetLookVector(), speed);
		finalMovement += -this->mCamera->GetLookVector();
	}
	if (kb.A)
	{
		//this->mCamera->MoveCamera(DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), speed);
		//this->mCamera->MoveCamera(-this->mCamera->GetRightVector(), speed);
		finalMovement += -this->mCamera->GetRightVector();
	}
	if (kb.D)
	{
		//this->mCamera->MoveCamera(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), speed);
		//this->mCamera->MoveCamera(this->mCamera->GetRightVector(), speed);
		finalMovement += this->mCamera->GetRightVector();
	}
	if (kb.Space)
	{
		//this->mCamera->MoveCamera(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), speed);
		finalMovement += DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	}
	if (kb.X)
	{
		//this->mCamera->MoveCamera(DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), speed);
		finalMovement += DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	}
	if (kb.LeftControl)
	{
		if (mouse.scrollWheelValue > lastscroll)
		{
			scale *= 1.1f;
		}
		else if (mouse.scrollWheelValue < lastscroll)
		{
			scale *= 0.9f;
		}
		lastscroll = mouse.scrollWheelValue;

	}
	if (kb.LeftAlt)
	{
		if (mouse.scrollWheelValue > lastscroll)
		{
			rotation = fmod(rotation + 0.1f, (2.0f * DirectX::XM_PI));
		}
		else if (mouse.scrollWheelValue < lastscroll)
		{
			rotation = fmod(rotation - 0.1f, (2.0f * DirectX::XM_PI));
		}
		lastscroll = mouse.scrollWheelValue;
	}

	this->mCamera->MoveCamera(XMVector3Normalize(finalMovement), speed);

	if (mouse.positionMode == DirectX::Mouse::MODE_RELATIVE)
	{
		DirectX::XMVECTOR delta = DirectX::XMVectorSet((float)mouse.x, (float)mouse.y, 0.0f, 0.0f);

		this->m_pitch += mouse.y * 0.001f;
		this->m_yaw += mouse.x * 0.001f;

		 //limit pitch to straight up or straight down
		 //with a little fudge-factor to avoid gimbal lock
		float limit = DirectX::XM_PI / 2.0f - 0.01f;
		m_pitch = max(-limit, m_pitch);
		m_pitch = min(+limit, m_pitch);

		// keep longitude in sane range by wrapping
		if (m_yaw > DirectX::XM_PI)
		{
			m_yaw -= DirectX::XM_PI * 2.0f;
		}
		else if (m_yaw < -DirectX::XM_PI)
		{
			m_yaw += DirectX::XM_PI * 2.0f;
		}
		this->mCamera->RotateCameraPitchYawRoll2(m_pitch, m_yaw, 0.0f);
	}

	m_mouse->SetMode(mouse.leftButton ? DirectX::Mouse::MODE_RELATIVE : DirectX::Mouse::MODE_ABSOLUTE);
}
