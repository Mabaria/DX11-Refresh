#include "Renderer.h"

Renderer::Renderer()
{
	this->Init();
	this->CreateShadersAndInputLayout();
	this->CreateConstantBuffers();
	this->CreateVertexBuffers();
}

Renderer::~Renderer()
{
}

void Renderer::Frame()
{
	this->gameTimer.Tick();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	this->mDeviceContext->ClearRenderTargetView(this->mRenderTargetView, this->mClearColor);
	this->mDeviceContext->ClearDepthStencilView(
		this->mDepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);
	this->mDeviceContext->OMSetRenderTargets(1, &this->mRenderTargetView, this->mDepthStencilView);

	this->mDeviceContext->VSSetShader(this->mCubeVertexShader, NULL, 0);
	this->mDeviceContext->PSSetShader(this->mCubePixelShader, NULL, 0);
	this->mDeviceContext->RSSetState(this->mRasterState);
	this->mDeviceContext->IASetVertexBuffers(0, 1, &this->mCubeVertexBuffer, &stride, &offset);
	// Set the WVP buffer.
	this->mDeviceContext->VSSetConstantBuffers(0, 1, &this->mWVPBuffer);
	this->mDeviceContext->Draw(8, 0);


	HRESULT hr = this->mSwapChain->Present(1, 0);
}

bool Renderer::Init()
{
	HRESULT hr;
	//// Create D3D11 Device
	//HRESULT hr = D3D11CreateDevice(
	//	NULL,
	//	D3D_DRIVER_TYPE_HARDWARE,
	//	NULL,
	//	D3D11_CREATE_DEVICE_DEBUG || D3D11_CREATE_DEVICE_SINGLETHREADED,
	//	NULL,
	//	0,
	//	D3D11_SDK_VERSION,
	//	&mDevice,
	//	NULL,
	//	&mDeviceContext
	//);

	//if (FAILED(hr))
	//{
	//	MessageBox(0, L"D3D11CreateDevice failed.", 0, 0);
	//	return false;
	//}

	HWND activeWindow = GetActiveWindow();
	RECT activeWindowRect;
	GetWindowRect(activeWindow, &activeWindowRect);

	this->mAspectRatio = (float)(activeWindowRect.right - activeWindowRect.left) / (float)(activeWindowRect.bottom - activeWindowRect.top);

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

	return true;
}

bool Renderer::CreateVertexBuffers()
{
	Vertex vertices[] =
	{
	{ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)},
	{ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)},
	{ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
	{ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
	{ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
	{ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
	{ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)},
	{ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)}
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

	ID3D11Buffer* CubeVertexBuffer;
	HRESULT hr = this->mDevice->CreateBuffer(
		&vbd,
		&vinitData,
		&CubeVertexBuffer
	);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateBuffer for cube vertex buffer failed", 0, 0);
		return false;
	}

	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
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
	HRESULT hr = D3DCompileFromFile(
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

	// Create input layout

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	hr = mDevice->CreateInputLayout(
		vertexDesc,
		2,
		vs_blob->GetBufferPointer(),
		vs_blob->GetBufferSize(),
		&this->mInputLayout
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateInputLayout failed", 0, 0);
		return false;
	}

	vs_blob->Release();
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

	return true;
}

bool Renderer::CreateConstantBuffers()
{
	DirectX::XMStoreFloat4x4(&mCubeWorld, DirectX::XMMatrixIdentity());


	DirectX::XMVECTOR pos = DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&mView, view);

	DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, this->mAspectRatio, 1.0f, 1000.0f);
	DirectX::XMStoreFloat4x4(&mProjection, proj);

	DirectX::XMMATRIX wvp = DirectX::XMMatrixMultiply(view, proj);

	VS_CONSTANT_BUFFER vsConstData;
	DirectX::XMStoreFloat4x4(&vsConstData.mWorldViewProj, wvp);
	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(VS_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
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
	return true;
}