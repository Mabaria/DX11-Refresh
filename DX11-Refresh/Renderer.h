#pragma once

#include "DX11-Refresh.h"
#include <d3d11.h>
#include <iostream>
#include <stdlib.h>
#include <Windows.h>
#include <vector>
#include <DirectXMath.h>
#include <DDSTextureLoader.h>
#include <d3dcompiler.h>
#include "Timer.h"
#include "Camera.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Obj_Loader.h"
#include "MeshObject.h"
#include <math.h>

#define MAX_NUMBER_OF_BONES_IN_SHADER 63

using namespace DirectX;

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
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Texcoord;
};

struct SkinVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Texcoord;
	DirectX::XMFLOAT3 BlendWeights;
	DirectX::XMINT4	  BlendIndices;
};

struct VS_WVP_CONSTANT_BUFFER
{
	DirectX::XMFLOAT4X4 mWorldViewProj;
};

struct VS_BONE_CONSTANT_BUFFER
{
	DirectX::XMFLOAT4X4 mBoneTransforms[MAX_NUMBER_OF_BONES_IN_SHADER];
};

class Renderer
{
public:
	Renderer();
	~Renderer();


	void Frame();
	void MouseMoved(int x, int y);
	void KeyPressed(WPARAM key);

	void LoadMesh(std::string &filepath);
	void LoadMesh(std::string& filepath, bool fbx);

private:
	Camera* mCamera = nullptr;
	float m_pitch = 0.0f;
	float m_yaw = 0.0f;

	ID3D11Buffer* sphereIndexBuffer;
	ID3D11Buffer* sphereVertBuffer;

	ID3D11VertexShader* SKYBOX_VS;
	ID3D11PixelShader* SKYBOX_PS;
	ID3D10Blob* SKYBOX_VS_Buffer;
	ID3D10Blob* SKYBOX_PS_Buffer;
	ID3D11SamplerState* skyboxSamplerState;
	ID3D11ShaderResourceView* smrv;

	ID3D11DepthStencilState* DSDefault;
	ID3D11DepthStencilState* DSLessEqual;
	ID3D11RasterizerState* RSCullNone;

	int NumSphereVertices;
	int NumSphereFaces;

	DirectX::XMMATRIX sphereWorld;

	XMMATRIX Rotationx;
	XMMATRIX Rotationy;
	XMMATRIX Rotationz;

	objl::Loader objLoader;
	std::vector<ID3D11Buffer*> testIndexBuffers;
	std::vector<ID3D11Buffer*> testVertexBuffers;
	std::vector<int> testIndexCount;

	std::vector<FbxLoader::Skeleton*> skinSkeletons;
	std::vector<std::vector<XMFLOAT4X4>> skinBoneMatrices;
	std::vector<ID3D11Buffer*> skinIndexBuffers;
	std::vector<ID3D11Buffer*> skinVertexBuffers;
	std::vector<int> skinIndexCount;

	ID3D11Buffer* mCubeVertexBuffer = nullptr;
	ID3D11Buffer* mCubeIndexBuffer = nullptr;
	ID3D11VertexShader* mCubeVertexShader = nullptr;
	ID3D11PixelShader* mCubePixelShader = nullptr;
	DirectX::XMFLOAT4X4 mCubeWorld;
	//DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProjection;
	ID3D11ShaderResourceView* mCubeTexSRV;

	float mAspectRatio = 0.0f;

	float mClearColor[4] = { 0.5f, 0.0f, 0.5f, 1.0f };

	ID3D11Buffer* mWVPBuffer = nullptr;
	ID3D11Buffer* mBoneTransformBuffer = nullptr;

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

	ID3D11InputLayout* mDefaultInputLayout;
	ID3D11InputLayout* mSkinInputLayout;

	ID3D11VertexShader* mSkinVertexShader = nullptr;

	bool Init();
	bool CreateVertexBuffers();
	bool CreateShadersAndInputLayout();
	bool CreateConstantBuffers();
	bool CreateSamplerState();
	bool CreateCubeMap();
	bool CreateDepthStencils();
	bool CreateFloorTexture();
	void CreateSphere(int LatLines, int LongLines);

	void ObjLoaderTest();

	void updateWVP(float dt);
	void HandleInput();

	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	std::unique_ptr<DirectX::Mouse> m_mouse;
};