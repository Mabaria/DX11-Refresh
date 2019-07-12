#pragma once

#include "DX11-Refresh.h"
#include <d3d11.h>
#include <iostream>
#include <stdlib.h>
#include <Windows.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "Timer.h"
#include "Camera.h"
#include "Keyboard.h"
#include "Mouse.h"

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

struct VS_CONSTANT_BUFFER
{
	DirectX::XMFLOAT4X4 mWorldViewProj;
};

class Renderer
{
public:
	Renderer();
	~Renderer();


	void Frame();
	void MouseMoved(int x, int y);
	void KeyPressed(WPARAM key);

private:
	Camera* mCamera = nullptr;
	float m_pitch = 0.0f;
	float m_yaw = 0.0f;


	ID3D11Buffer* mCubeVertexBuffer = nullptr;
	ID3D11Buffer* mCubeIndexBuffer = nullptr;
	ID3D11VertexShader* mCubeVertexShader = nullptr;
	ID3D11PixelShader* mCubePixelShader = nullptr;
	DirectX::XMFLOAT4X4 mCubeWorld;
	//DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProjection;

	float mAspectRatio = 0.0f;

	float mClearColor[4] = { 0.5f, 0.0f, 0.5f, 1.0f };

	ID3D11Buffer* mWVPBuffer = nullptr;

	ID3D11RasterizerState* mRasterState = nullptr;

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
	bool CreateConstantBuffers();

	void updateWVP(float dt);
	void HandleInput();

	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	std::unique_ptr<DirectX::Mouse> m_mouse;
};