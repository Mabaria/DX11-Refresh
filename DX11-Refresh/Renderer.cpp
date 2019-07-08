#include "Renderer.h"

Renderer::Renderer()
{
	this->Init();
	this->CreateShadersAndInputLayout();
}

Renderer::~Renderer()
{
}

void Renderer::Frame()
{
	this->gameTimer.Tick();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	this->mDeviceContext->IASetVertexBuffers(0, 1, &this->mCubeVertexBuffer, &stride, &offset);
	this->mDeviceContext->Draw(8, 0);
}

bool Renderer::Init()
{
	// Create D3D11 Device
	HRESULT hr = D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG || D3D11_CREATE_DEVICE_SINGLETHREADED,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&mDevice,
		NULL,
		&mDeviceContext
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice failed.", 0, 0);
		return false;
	}

	HWND activeWindow = GetActiveWindow();
	RECT activeWindowRect;
	GetWindowRect(activeWindow, &activeWindowRect);

	DXGI_MODE_DESC modeDesc;
	modeDesc.Width						= activeWindowRect.right - activeWindowRect.left;
	modeDesc.Height						= activeWindowRect.bottom - activeWindowRect.top;
	modeDesc.RefreshRate.Numerator		= 60; // Refresh rate set to 60
	modeDesc.RefreshRate.Denominator	= 1;
	modeDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	modeDesc.ScanlineOrdering			= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	modeDesc.Scaling					= DXGI_MODE_SCALING_UNSPECIFIED;

	// checking for multisample support
	UINT numQualityLevels				= 0;

	mDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &numQualityLevels);

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc			= modeDesc;
	swapChainDesc.SampleDesc.Count		= 4;
	swapChainDesc.SampleDesc.Quality	= numQualityLevels - 1;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount			= 1;
	swapChainDesc.Windowed				= true;
	swapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags					= 0;
	swapChainDesc.OutputWindow			= GetActiveWindow();

	// Get dxgiFactory for creation of swapchain 

	IDXGIDevice* dxgiDevice = 0;
	mDevice->QueryInterface(__uuidof(IDXGIDevice),
		(void**)& dxgiDevice);

	IDXGIAdapter* dxgiAdapter = 0;
	dxgiDevice->GetAdapter(&dxgiAdapter);

	IDXGIFactory* dxgiFactory = 0;
	dxgiAdapter->GetParent(__uuidof(IDXGIFactory),
		(void**)& dxgiFactory);

	dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();


	hr = dxgiFactory->CreateSwapChain(mDevice, &swapChainDesc, &mSwapChain);

	if (FAILED(hr))
	{
		MessageBox(0, L"CreateSwapChain failed.", 0, 0);
		return false;
	}

	ID3D11RenderTargetView* mRenderTargetView;
	ID3D11Texture2D* backBuffer;
	hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&backBuffer));

	if (FAILED(hr))
	{
		MessageBox(0, L"GetBuffer failed.", 0, 0);
		return false;
	}
	if (backBuffer != nullptr)
	{
		hr = mDevice->CreateRenderTargetView(backBuffer, 0, &mRenderTargetView);

		if (FAILED(hr))
		{
			MessageBox(0, L"CreateRenderTargetView failed.", 0, 0);
			return false;
		}
	}
	
	backBuffer->Release();

	// Create depth stencil buffer and view

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width					= activeWindowRect.right - activeWindowRect.left;
	depthStencilDesc.Height					= activeWindowRect.bottom - activeWindowRect.top;
	depthStencilDesc.MipLevels				= 1;
	depthStencilDesc.ArraySize				= 1;
	depthStencilDesc.Format					= DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 4x MSAA
	depthStencilDesc.SampleDesc.Count		= 4;
	depthStencilDesc.SampleDesc.Quality		= numQualityLevels - 1;

	depthStencilDesc.Usage					= D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags				= D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags			= 0;
	depthStencilDesc.MiscFlags				= 0;

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
