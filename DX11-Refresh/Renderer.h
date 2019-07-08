#pragma once

#include "DX11-Refresh.h"
#include <d3d11.h>
#include <iostream>
#include <Windows.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "Timer.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};
class Renderer
{
public:
	Renderer();
	~Renderer();


	void Frame();

private:
	ID3D11Buffer* mCubeVertexBuffer = nullptr;
	ID3D11VertexShader* mCubeVertexShader = nullptr;
	ID3D11PixelShader* mCubePixelShader = nullptr;

	Timer gameTimer;
	ID3D11Device* mDevice = nullptr;
	ID3D11DeviceContext* mDeviceContext = nullptr;
	ID3D11RenderTargetView* mRenderTargetView = nullptr;
	ID3D11Texture2D* mBackBuffer = nullptr;
	IDXGISwapChain* mSwapChain = nullptr;
	ID3D11Texture2D* mDepthStencilBuffer = nullptr;
	ID3D11DepthStencilView* mDepthStencilView = nullptr;
	D3D11_VIEWPORT vp;

	ID3D11InputLayout* mInputLayout;

	bool Init();
	bool CreateVertexBuffers();
	bool CreateShadersAndInputLayout();
};