#include "Camera.h"

Camera::Camera(const DirectX::XMVECTOR& r_position,
	const DirectX::XMVECTOR& r_up_vector,
	const DirectX::XMVECTOR& r_look_vector,
	const float view_width_or_fov_angle,
	const float view_height_or_aspect_ratio,
	const float near_z,
	const float far_z,
	const LOOK_MODE camera_mode,
	const PROJECTION_MODE projection_mode)
{
	this->mInit(r_position,
		r_up_vector,
		r_look_vector,
		view_width_or_fov_angle,
		view_height_or_aspect_ratio,
		near_z,
		far_z,
		camera_mode,
		projection_mode);
}

Camera::Camera(const DirectX::XMFLOAT3& r_position,
	const DirectX::XMFLOAT3& r_up_vector,
	const DirectX::XMFLOAT3& r_look_vector,
	const float view_width_or_fov_angle,
	const float view_height_or_aspect_ratio,
	const float near_z,
	const float far_z,
	const LOOK_MODE camera_mode,
	const PROJECTION_MODE projection_mode)
{
	this->mInit(DirectX::XMLoadFloat3(&r_position),
		DirectX::XMLoadFloat3(&r_up_vector),
		DirectX::XMLoadFloat3(&r_look_vector),
		view_width_or_fov_angle,
		view_height_or_aspect_ratio,
		near_z,
		far_z,
		camera_mode,
		projection_mode);
}

Camera::Camera(const float pos_x,
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
	const LOOK_MODE camera_mode,
	const PROJECTION_MODE projection_mode)
{
	this->mInit(DirectX::XMVectorSet(pos_x, pos_y, pos_z, 0.0f),
		DirectX::XMVectorSet(up_x, up_y, up_z, 0.0f),
		DirectX::XMVectorSet(look_x, look_y, look_z, 0.0f),
		view_width_or_fov_angle,
		view_height_or_aspect_ratio,
		near_z,
		far_z,
		camera_mode,
		projection_mode);
}

Camera::~Camera()
{

}

void Camera::mReset()
{
	this->mInit(
		this->mDefaultValues.pos,
		this->mDefaultValues.up,
		this->mDefaultValues.look,
		this->mDefaultValues.widthOrFov,
		this->mDefaultValues.heightOrRatio,
		this->mDefaultValues.nearZ,
		this->mDefaultValues.farZ,
		this->mDefaultValues.cameraMode,
		this->mDefaultValues.projMode);
}

void Camera::mInit(const DirectX::XMVECTOR& r_position,
	const DirectX::XMVECTOR& r_up_vector,
	const DirectX::XMVECTOR& r_look_vector,
	const float view_width_or_fov_angle,
	const float view_height_or_aspect_ratio,
	const float near_z,
	const float far_z,
	const LOOK_MODE camera_mode,
	const PROJECTION_MODE projection_mode)
{
	if (this->mDefaultValues.widthOrFov != view_width_or_fov_angle) {
		// This is the first time running init
		this->mDefaultValues = {
			r_position,
			r_up_vector,
			r_look_vector,
			view_width_or_fov_angle,
			view_height_or_aspect_ratio,
			near_z,
			far_z,
			camera_mode,
			projection_mode
		};
	}
	this->mCameraPosition = r_position;
	this->mUpVector = r_up_vector;
	this->mLookVector = r_look_vector;
	this->mViewWidth = view_width_or_fov_angle;
	this->mFovAngle = view_width_or_fov_angle;
	this->mViewHeight = view_height_or_aspect_ratio;
	this->mAspectRatio = view_height_or_aspect_ratio;
	this->mNearZ = near_z;
	this->mFarZ = far_z;
	this->mLookMode = camera_mode;
	this->mProjectionMode = projection_mode;
	this->mUpdateViewMatrix();
	this->mUpdateProjMatrix();
}

void Camera::mUpdateViewMatrix()
{
	if (this->mLookMode == LOOK_AT)
	{
		this->mViewMatrix = DirectX::XMMatrixLookAtLH(this->mCameraPosition,
			this->mLookVector,
			this->mUpVector);
	}
	else
	{
		this->mViewMatrix = DirectX::XMMatrixLookToLH(this->mCameraPosition,
			this->mLookVector,
			this->mUpVector);
	}
}

void Camera::mUpdateProjMatrix()
{
	if (this->mProjectionMode == PERSPECTIVE)
	{
		this->mProjMatrix = DirectX::XMMatrixPerspectiveFovLH(
			this->mFovAngle,
			this->mAspectRatio,
			this->mNearZ,
			this->mFarZ
		);
	}
	else
	{
		this->mProjMatrix = DirectX::XMMatrixOrthographicLH(
			this->mViewWidth,
			this->mViewHeight,
			this->mNearZ,
			this->mFarZ
		);
	}

}

void Camera::mRotateViewMatrix(const DirectX::XMMATRIX& camRotationMatrix)
{
	//! IF SOMETHING IS BROKEN WITH CAMERA ROTATION THIS IS PROBABLY THE ROOT CAUSE
	this->mLookVector =
		DirectX::XMVector3TransformCoord(this->mLookVector, camRotationMatrix);
	this->mUpVector =
		DirectX::XMVector3TransformCoord(this->mUpVector, camRotationMatrix);
	this->mUpdateViewMatrix();
}

void Camera::SetCameraPosition(const DirectX::XMFLOAT3& r_new_position)
{
	this->mCameraPosition = DirectX::XMLoadFloat3(&r_new_position);
	this->mUpdateViewMatrix();
}

void Camera::SetCameraPosition(const DirectX::XMVECTOR& r_new_position)
{
	this->mCameraPosition = r_new_position;
	this->mUpdateViewMatrix();
}

void Camera::SetCameraPosition(const float new_x, const float new_y,
	const float new_z)
{
	this->mCameraPosition = DirectX::XMVectorSet(new_x, new_y, new_z, 0.0f);
	this->mUpdateViewMatrix();
}

void Camera::MoveCamera(const DirectX::XMFLOAT3& r_direction,
	const float distance)
{
	DirectX::XMVECTOR direction_vector = DirectX::XMLoadFloat3(&r_direction);
	DirectX::XMVECTOR displacement =
		DirectX::XMVectorScale(direction_vector, distance);

	this->SetCameraPosition(
		DirectX::XMVectorAdd(this->mCameraPosition, displacement));
	this->SetLookVector(DirectX::XMVectorAdd(this->mLookVector, displacement));

}

void Camera::MoveCamera(const DirectX::XMVECTOR& r_direction,
	const float distance)
{
	DirectX::XMVECTOR displacement =
		DirectX::XMVectorScale(r_direction, distance);

	this->SetCameraPosition(
		DirectX::XMVectorAdd(this->mCameraPosition, displacement));
	this->SetLookVector(DirectX::XMVectorAdd(this->mLookVector, displacement));
}

void Camera::MoveCamera(const float direction_x,
	const float direction_y,
	const float direction_z,
	const float distance)
{
	DirectX::XMVECTOR displacement =
		DirectX::XMVectorScale(
			DirectX::XMVectorSet(direction_x,
				direction_y,
				direction_z,
				0.0f),
			distance);
	this->SetCameraPosition(
		DirectX::XMVectorAdd(this->mCameraPosition, displacement));
	this->SetLookVector(DirectX::XMVectorAdd(this->mLookVector, displacement));
}

void Camera::SetUpVector(const DirectX::XMFLOAT3& r_new_up)
{
	this->mUpVector = DirectX::XMLoadFloat3(&r_new_up);
	this->mUpdateViewMatrix();
}

void Camera::SetUpVector(const DirectX::XMVECTOR& r_new_up)
{
	this->mUpVector = r_new_up;
	this->mUpdateViewMatrix();
}

void Camera::SetUpVector(const float new_x,
	const float new_y,
	const float new_z)
{
	this->mUpVector = DirectX::XMVectorSet(new_x, new_y, new_z, 0.0f);
	this->mUpdateViewMatrix();
}

void Camera::SetLookVector(const DirectX::XMFLOAT3& r_new_look_vector)
{
	this->mLookVector = DirectX::XMLoadFloat3(&r_new_look_vector);
	this->mUpdateViewMatrix();
}

void Camera::SetLookVector(const DirectX::XMVECTOR& r_new_look_vector)
{
	this->mLookVector = r_new_look_vector;
	this->mUpdateViewMatrix();
}

void Camera::SetLookVector(const float new_x, const float new_y, const float new_z)
{
	this->mLookVector = DirectX::XMVectorSet(new_x, new_y, new_z, 0.0f);
	this->mUpdateViewMatrix();
}

void Camera::SetLookMode(const LOOK_MODE new_look_mode)
{
	this->mLookMode = new_look_mode;
}

void Camera::RotateCameraPitchYawRoll(const float pitch,
	const float yaw,
	const float roll)
{
	this->mRotateViewMatrix(
		DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll));

}

void Camera::RotateCameraPitchYawRoll(const DirectX::XMFLOAT3& pitch_yaw_roll)
{
	this->mRotateViewMatrix(
		DirectX::XMMatrixRotationRollPitchYawFromVector(
			DirectX::XMLoadFloat3(&pitch_yaw_roll)));
}

void Camera::RotateCameraPitchYawRoll(const DirectX::XMVECTOR& pitch_yaw_roll)
{
	this->mRotateViewMatrix(
		DirectX::XMMatrixRotationRollPitchYawFromVector(pitch_yaw_roll));
}

float Camera::GetViewWidth() const
{
	if (this->mProjectionMode == PERSPECTIVE)
		return (float)(2 * this->mNearZ *
			tan((this->mFovAngle * DirectX::XM_PI) / (2 * 180)));

	// Else return Orthografic
	return this->mViewWidth;
}

float Camera::GetViewHeight() const
{
	if (this->mProjectionMode == PERSPECTIVE)
		return this->GetViewWidth() / this->mAspectRatio;

	// Else return Orthografic
	return this->mViewHeight;
}

float Camera::GetNearZ() const
{
	return this->mNearZ;
}

float Camera::GetFarZ() const
{
	return this->mFarZ;
}

DirectX::XMVECTOR Camera::GetPosition() const
{
	return this->mCameraPosition;
}

DirectX::XMVECTOR Camera::GetUpVector() const
{
	return this->mUpVector;
}

DirectX::XMVECTOR Camera::GetLookVector() const
{
	return this->mLookVector;
}

DirectX::XMVECTOR Camera::GetLookToVector() const
{
	if (this->mLookMode == LOOK_AT)
	{
		return DirectX::XMVector3Normalize(
			DirectX::XMVectorSubtract(this->mLookVector, this->mCameraPosition));
	}

	// Else
	return this->mLookVector;
}

LOOK_MODE Camera::GetLookMode() const
{
	return this->mLookMode;
}

float Camera::GetAspectRatio() const
{
	return this->mAspectRatio;
}

DirectX::XMVECTOR Camera::GetRightVector() const
{
	return DirectX::XMVector3Cross(this->mUpVector, this->GetLookToVector());
}

DirectX::XMMATRIX Camera::GetViewMatrix() const
{
	return this->mViewMatrix;
}

DirectX::XMMATRIX Camera::GetTransposedViewMatrix() const
{
	return XMMatrixTranspose(this->mViewMatrix);
}

DirectX::XMMATRIX Camera::GetProjectionMatrix() const
{
	return this->mProjMatrix;
}

DirectX::XMMATRIX Camera::GetTransposedProjectionMatrix() const
{
	return DirectX::XMMatrixTranspose(this->mProjMatrix);
}

PROJECTION_MODE Camera::GetProjectionMode() const
{
	return this->mProjectionMode;
}

bool Camera::SetViewWidth(const float new_view_width)
{
	if (new_view_width >= 0)
	{
		this->mViewWidth = new_view_width;
		this->mUpdateProjMatrix();
		return true;
	}
	else
	{
		return false;
	}
}

bool Camera::SetViewHeight(const float new_view_height)
{
	if (new_view_height >= 0)
	{
		this->mViewHeight = new_view_height;
		this->mUpdateProjMatrix();
		return true;
	}
	else
	{
		return false;
	}
}

bool Camera::SetFovAngle(const float new_fov_angle)
{
	if (new_fov_angle >= 0)
	{
		this->mFovAngle = new_fov_angle;
		this->mUpdateProjMatrix();
		return true;
	}
	else
	{
		return false;
	}
}

bool Camera::SetAspectRatio(const float new_aspect_ratio)
{
	if (new_aspect_ratio >= 0)
	{
		this->mAspectRatio = new_aspect_ratio;
		this->mUpdateProjMatrix();
		return true;
	}
	else
	{
		return false;
	}
}

bool Camera::SetNearZ(const float new_near_z)
{
	if (new_near_z != this->mFarZ) // Near and far cannot be equal
	{
		this->mNearZ = new_near_z;
		this->mUpdateProjMatrix();
		return true;
	}
	else
	{
		return false;
	}
}

bool Camera::SetFarZ(const float new_far_z)
{
	if (new_far_z != this->mNearZ)
	{
		this->mFarZ = new_far_z;
		this->mUpdateProjMatrix();
		return true;
	}
	else
	{
		return false;
	}
}

void Camera::SetProjectionMode(const PROJECTION_MODE new_projection_mode)
{
	this->mProjectionMode = new_projection_mode;
	this->mUpdateProjMatrix();
}

void Camera::Update()
{
	// Reset the camera when prompted by button
	this->mReset();
	//this->NotifyObservers(&std::string("reset"));
}

void Camera::Reset()
{
	this->mReset();
	//this->NotifyObservers(&std::string("resetA"));
}

CAMERA_DEFAULT_VALUES Camera::GetDefaultValues()
{
	return this->mDefaultValues;
}