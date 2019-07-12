#pragma once
#include <DirectXMath.h>

enum LOOK_MODE {
	LOOK_AT = 0,
	LOOK_TO = 1
};

enum PROJECTION_MODE {
	PERSPECTIVE = 0,
	ORTHOGRAPHIC = 1
};

struct CAMERA_DEFAULT_VALUES {
	DirectX::XMVECTOR pos, up, look;
	float widthOrFov, heightOrRatio, nearZ, farZ;
	LOOK_MODE cameraMode;
	PROJECTION_MODE projMode;
};

class Camera {

public:
	Camera(const DirectX::XMVECTOR& r_position,
		const DirectX::XMVECTOR& r_up_vector,
		const DirectX::XMVECTOR& r_look_vector,
		const float view_width_or_fov_angle,
		const float view_height_or_aspect_ratio,
		const float near_z,
		const float far_z,
		const LOOK_MODE camera_mode = LOOK_AT,
		const PROJECTION_MODE projection_mode = PERSPECTIVE);

	Camera(const DirectX::XMFLOAT3& r_position,
		const DirectX::XMFLOAT3& r_up_vector,
		const DirectX::XMFLOAT3& r_look_vector,
		const float view_width_or_fov_angle,
		const float view_height_or_aspect_ratio,
		const float near_z,
		const float far_z,
		const LOOK_MODE camera_mode = LOOK_AT,
		const PROJECTION_MODE projection_mode = PERSPECTIVE);

	Camera(const float pos_x,
		const float pos_y,
		const float pos_z,
		const float up_x,
		const float up_y,
		const float up_z,
		const float look_x,
		const float look_y,
		const float look_z,
		const float view_width_or_fov_angle,
		const float view_height_or_aspect_ratio,
		const float near_z,
		const float far_z,
		const LOOK_MODE camera_mode = LOOK_AT,
		const PROJECTION_MODE projection_mode = PERSPECTIVE);

	~Camera();

	void SetCameraPosition(const DirectX::XMFLOAT3& r_new_position);
	void SetCameraPosition(const DirectX::XMVECTOR& r_new_position);
	void SetCameraPosition(const float new_x,
		const float new_y,
		const float new_z);

	void* operator new(size_t i) // To make sure it is 16 bit aligned
	{
		return _aligned_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_aligned_free(p);
	}

	// MoveCamera takes a direction vector and a magnitude to move along that vector.
	// The function does not normalize the vector, so if the vector is wonky that's
	// on you.

	void MoveCamera(const DirectX::XMFLOAT3& r_direction,
		const float distance);
	void MoveCamera(const DirectX::XMVECTOR& r_direction,
		const float distance);
	void MoveCamera(const float direction_x,
		const float direction_y,
		const float direction_z,
		const float distance);

	void SetUpVector(const DirectX::XMFLOAT3& r_new_up);
	void SetUpVector(const DirectX::XMVECTOR& r_new_up);
	void SetUpVector(const float new_x, const float new_y, const float new_z);

	void SetLookVector(const DirectX::XMFLOAT3& r_new_look_vector);
	void SetLookVector(const DirectX::XMVECTOR& r_new_look_vector);
	void SetLookVector(const float new_x, const float new_y, const float new_z);

	void SetLookMode(const LOOK_MODE new_look_mode);

	void RotateCameraPitchYawRoll(
		const float pitch,
		const float yaw,
		const float roll);
	void RotateCameraPitchYawRoll(const DirectX::XMFLOAT3& pitch_yaw_roll);
	void RotateCameraPitchYawRoll(const DirectX::XMVECTOR& pitch_yaw_roll);

	void RotateCameraPitchYawRoll2(
		const float pitch,
		const float yaw,
		const float roll);

	float GetViewWidth() const;
	float GetViewHeight() const;
	float GetNearZ() const;
	float GetFarZ() const;

	DirectX::XMVECTOR GetPosition() const;
	DirectX::XMVECTOR GetUpVector() const;
	DirectX::XMVECTOR GetLookVector() const;
	DirectX::XMVECTOR GetLookToVector() const;
	LOOK_MODE GetLookMode() const;
	float GetAspectRatio() const;

	DirectX::XMVECTOR GetRightVector() const;

	DirectX::XMMATRIX GetViewMatrix() const;
	DirectX::XMMATRIX GetTransposedViewMatrix() const;

	DirectX::XMMATRIX GetProjectionMatrix() const;
	DirectX::XMMATRIX GetTransposedProjectionMatrix() const;
	PROJECTION_MODE GetProjectionMode() const;

	bool SetViewWidth(const float new_view_width); // Not used in PERSPECTIVE mode
	bool SetViewHeight(const float new_view_height); // Not used in PERSPECTIVE mode
	bool SetFovAngle(const float new_fov_angle); // Not used in ORTHOGRAPHIC mode
	bool SetAspectRatio(const float new_aspect_ratio); // Not used in ORTHOGRAPHIC mode
	bool SetNearZ(const float new_near_z);
	bool SetFarZ(const float new_far_z);

	void SetProjectionMode(const PROJECTION_MODE new_projection_mode);

	void Update();
	void Reset();
	CAMERA_DEFAULT_VALUES GetDefaultValues();


private:
	DirectX::XMVECTOR mCameraPosition;
	DirectX::XMVECTOR mUpVector;
	DirectX::XMVECTOR mLookVector;

	float mViewWidth; // Not used in PERSPECTIVE mode
	float mViewHeight; // Not used in PERSPECTIVE mode
	float mFovAngle; // Not used in ORTHOGRAPHIC mode
	float mAspectRatio; // Not used in ORTHOGRAPHIC mode
	float mNearZ;
	float mFarZ;
	LOOK_MODE mLookMode;
	PROJECTION_MODE mProjectionMode;
	DirectX::XMMATRIX mViewMatrix;
	DirectX::XMMATRIX mProjMatrix;

	CAMERA_DEFAULT_VALUES mDefaultValues;

	void mReset();
	void mInit(const DirectX::XMVECTOR& r_position,
		const DirectX::XMVECTOR& r_up_vector,
		const DirectX::XMVECTOR& r_look_vector,
		const float view_width_or_fov_angle,
		const float view_height_or_aspect_ratio,
		const float near_z,
		const float far_z,
		const LOOK_MODE camera_mode,
		const PROJECTION_MODE projection_mode);
	void mUpdateViewMatrix();
	void mUpdateProjMatrix();
	void mRotateViewMatrix(const DirectX::XMMATRIX& camRotationMatrix);
	//! IF SOMETHING IS BROKEN WITH CAMERA ROTATION
	//! mRotateViewMatrix IS PROBABLY THE ROOT CAUSE

};