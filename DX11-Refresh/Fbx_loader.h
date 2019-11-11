#pragma once

#include <fbxsdk.h>
#include <vector>
#include <d3d11.h>
#include <cassert>
#include <map>
#include <unordered_map>
#include <DirectXMath.h>
#include <memory>
#include <algorithm>
#include <locale>

#define MAX_NUM_WEIGHTS_PER_VERTEX 4
#define ANIMATION_CROSSFADE_DURATION 1.0f

namespace FbxLoader
{

	struct FbxVertex
	{
		float pos[3];
		float nor[3];
		float uv[2];
	};

	//This stores the weight of each Control Point
	struct IndexWeightPair
	{
		unsigned int index;	//index of joint 
		double weight;		//weight of influence by the joint
		IndexWeightPair() :
			index(0), weight(0.0)
		{}
	};


	// Each vertex contains up to four IndexWeightPairs
	struct ControlPointInfo
	{
		IndexWeightPair weightPairs[4];
	};

	//This stores the information of each key frame of each Joint
	struct KeyFrame {
		FbxLongLong mFrameNum;
		FbxAMatrix mGlobalTransform;	//global transform not including bindpose inverse transform matrix
		FbxAMatrix mLocalTransform;
		DirectX::XMFLOAT4X4 mOffsetMatrix; // final offset matrix, this is what needs to be sent to the GPU
	};

	struct Joint {
		std::string mName;
		int mParentIndex;	//index to its parent joint
		std::vector<KeyFrame> mAnimationVector;
		FbxNode* mNode;

		std::vector<unsigned int> mConnectedVertexIndices;

		FbxAMatrix mGlobalBindposeInverse;
		FbxAMatrix mBoneGlobalTransform;
		FbxAMatrix mOffsetMatrix;

		Joint() :
			mNode(nullptr)
		{
			mParentIndex = -1;
		}
	};

	enum ANIMATION_TYPE
	{
		PING,
		IDLE,
		MOVE,
		ATTACK,
		ANIMATION_COUNT
	};

	struct AnimationSet
	{
		DirectX::XMFLOAT4X4* animationData;
		unsigned int frameCount;
		unsigned int activeFrame = 0;
		std::string animationName;
	};



	struct Skeleton {
	private:
		float mCurrentTime = 0;

	public:
		std::vector<Joint> joints;
		DirectX::XMFLOAT4X4* animationData; // LEGACY, please use UpdateAnimation and frameData instead
		DirectX::XMFLOAT4X4* frameData;
		unsigned int jointCount = 0;
		unsigned int frameCount = 0;
		AnimationSet animations[ANIMATION_COUNT];
		int animationFlags[ANIMATION_COUNT]; // -1 : missing animation
											 // 0  : disabled
											 // 1  : enabled
		Skeleton()
		{
			// Initialize the animation flags with -1 for missing animation
			std::fill(animationFlags, animationFlags + ANIMATION_COUNT, -1);
		}
		// Currently does not blend animations, simply updates the global animationData with info from the first enabled animation.
		void UpdateAnimation(float dtInSeconds)
		{

			for (int i = 0; i < ANIMATION_COUNT; ++i)
			{
				if (this->animationFlags[i] == 1)
				{
					this->animationData = animations[i].animationData;
					this->frameCount = animations[i].frameCount;
					this->mCurrentTime = this->mCurrentTime + dtInSeconds;
					int frame_to_set = (int)std::round(fmod(this->mCurrentTime * 60.0f, this->frameCount)) % this->frameCount;
					// Get frame data from animationData vector
					memcpy(this->frameData, &animations[i].animationData[frame_to_set * this->jointCount], this->jointCount * sizeof(DirectX::XMFLOAT4X4));
					break; // This break will disappear when animation blending is implemented
				}
			}
		}
		// Returns false if requested animation does not exist
		bool StartAnimation(ANIMATION_TYPE anim_type)
		{
			if (anim_type < ANIMATION_COUNT)
			{
				if (this->animationFlags[anim_type] != -1)
				{
					this->animationFlags[anim_type] = 1;
					this->animations[anim_type].activeFrame = 0;
					return true;
				}
			}
			return false;
		}
		// Returns false if requested animation does not exist
		bool StopAnimation(ANIMATION_TYPE anim_type)
		{
			if (anim_type < ANIMATION_COUNT)
			{
				if (this->animationFlags[anim_type] != -1)
				{
					this->animationFlags[anim_type] = 0;
					return true;
				}
			}
			return false;
		}
	};

	struct UniqueSkeletonData
	{
	private:
		float mCurrentTime = 0.0f;
		float mPrevTime = 0.0f;
		// Default to idle
		int mActiveAnimation = (int)FbxLoader::ANIMATION_TYPE::IDLE;
		int mPrevAnimation = -1;

		float mPrevAnimTransitionTime = -1.0f;
	public:

		Skeleton* parentSkeleton;
		DirectX::XMFLOAT4X4* frameData;
		int animationFlags[ANIMATION_COUNT];
		UniqueSkeletonData()
		{

		}
		void ResetFrameCount()
		{
			this->mCurrentTime = 0.0f + static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		}
		void Init(Skeleton* parentSkeleton)
		{
			this->parentSkeleton = parentSkeleton;
			this->frameData = new DirectX::XMFLOAT4X4[parentSkeleton->jointCount];
			for (int i = 0; i < ANIMATION_COUNT; ++i)
			{
				this->animationFlags[i] = parentSkeleton->animationFlags[i];
			}
		}
		void UpdateAnimation(float dtInSeconds, ANIMATION_TYPE animType)
		{
			// If we are swapping animation
			if (mActiveAnimation != (int)animType)
			{
				// This prevents super quick switching of animations, and only updates prevanimation if the
				// current animation has been playing for AT LEAST a few frames
				if (mPrevAnimTransitionTime < ANIMATION_CROSSFADE_DURATION - 0.05f)
				{
					mPrevTime = mCurrentTime;
					ResetFrameCount();
					mPrevAnimation = mActiveAnimation;
				}

				mActiveAnimation = (int)animType;
				mPrevAnimTransitionTime = ANIMATION_CROSSFADE_DURATION;
			}

			if (this->animationFlags[animType] != -1)
			{
				int frame_count = this->parentSkeleton->animations[animType].frameCount;
				int joint_count = this->parentSkeleton->jointCount;
				this->mCurrentTime = this->mCurrentTime + dtInSeconds;
				int frame_to_set = (int)std::round(fmod(this->mCurrentTime * 60.0f, frame_count)) % frame_count;
				// Get frame data from animationData vector
				memcpy(this->frameData, &this->parentSkeleton->animations[animType].animationData[frame_to_set * joint_count], joint_count * sizeof(DirectX::XMFLOAT4X4));

				// If we are currently transitioning between two animations
				if (mPrevAnimTransitionTime >= 0.0f)
				{
					mPrevTime += dtInSeconds;
					float prev_weight = mPrevAnimTransitionTime / ANIMATION_CROSSFADE_DURATION;
					//float current_weight = 1.0f - prev_weight;
					int prev_frame_count = this->parentSkeleton->animations[mPrevAnimation].frameCount;
					DirectX::XMFLOAT4X4* prevFrameData = new DirectX::XMFLOAT4X4[parentSkeleton->jointCount];
					int prev_frame_to_set = (int)std::round(fmod(this->mPrevTime * 60.0f, prev_frame_count)) % prev_frame_count;
					// Get frame data from animationData vector for the PREVIOUS animation
					// Same procedure as current animation
					memcpy(prevFrameData,
						&this->parentSkeleton->animations[mPrevAnimation].animationData[prev_frame_to_set * joint_count],
						joint_count * sizeof(DirectX::XMFLOAT4X4));
					// For each joint, decompose the two animation matrices and interpolate them, combine to a "final" animation matrix
					for (unsigned int i = 0; i < this->parentSkeleton->jointCount; ++i)
					{
						DirectX::XMVECTOR current_frame_scale, current_frame_rot, current_frame_trans;
						DirectX::XMVECTOR prev_frame_scale, prev_frame_rot, prev_frame_trans;
						DirectX::XMVECTOR new_frame_scale, new_frame_rot, new_frame_trans;
						// Transposing both matrices as they are currently in shader-readable format
						DirectX::XMMATRIX current_frame_matrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&this->frameData[i]));
						DirectX::XMMATRIX prev_frame_matrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&prevFrameData[i]));
						// Decompose the matrix in order to be able to interpolate the components
						DirectX::XMMatrixDecompose(&current_frame_scale, &current_frame_rot, &current_frame_trans, current_frame_matrix);
						DirectX::XMMatrixDecompose(&prev_frame_scale, &prev_frame_rot, &prev_frame_trans, prev_frame_matrix);

						// Calculate the interpolated components of the new matrix
						new_frame_rot = DirectX::XMQuaternionSlerp(current_frame_rot, prev_frame_rot, prev_weight);
						new_frame_scale = DirectX::XMVectorLerp(current_frame_scale, prev_frame_scale, prev_weight);
						new_frame_trans = DirectX::XMVectorLerp(current_frame_trans, prev_frame_trans, prev_weight);

						// Transposing the new matrix to convert it back to shader readable format
						DirectX::XMMATRIX new_frame_matrix = DirectX::XMMatrixTranspose(
							DirectX::XMMatrixScalingFromVector(new_frame_scale) *
							DirectX::XMMatrixRotationQuaternion(new_frame_rot) *
							DirectX::XMMatrixTranslationFromVector(new_frame_trans));
						// Store the new matrix in framedata
						XMStoreFloat4x4(&this->frameData[i], new_frame_matrix);
					}
					// Reduce the transition timer
					mPrevAnimTransitionTime -= dtInSeconds;
					delete[] prevFrameData;
				}
			}
		}
		// Returns false if requested animation does not exist
		bool StartAnimation(ANIMATION_TYPE anim_type)
		{
			if (anim_type < ANIMATION_COUNT)
			{
				if (this->animationFlags[anim_type] != -1)
				{
					this->animationFlags[anim_type] = 1;
					this->mCurrentTime = 0.0f;
					return true;
				}
			}
			return false;
		}
		// Returns false if requested animation does not exist
		bool StopAnimation(ANIMATION_TYPE anim_type)
		{
			if (anim_type < ANIMATION_COUNT)
			{
				if (this->animationFlags[anim_type] != -1)
				{
					this->animationFlags[anim_type] = 0;
					return true;
				}
			}
			return false;
		}
		DirectX::XMFLOAT4X4 GetOffsetMatrixUsingJointName(const std::string& inJointName)
		{
			int joint_index = -1;
			for (unsigned int i = 0; i < this->parentSkeleton->joints.size(); ++i)
			{
				if (this->parentSkeleton->joints[i].mName == inJointName)
				{
					joint_index = i;
					break;
				}
			}

			if (joint_index >= 0) // if joint name existed
			{
				// Animation data layout reminder:
				// [[[ |FRAME 0| joint0mat joint1mat joint2mat ... jointnmat |FRAME 1| joint0mat joint1mat joint2mat ... jointnmat |FRAME 2| ....... ]]]
				return this->frameData[joint_index];
			}
			else
			{
				// Return error matrix
				return DirectX::XMFLOAT4X4(-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f);
			}
		}

		DirectX::XMFLOAT4X4 GetInverseBindPoseUsingJointName(const std::string& inJointName)
		{
			int joint_index = -1;
			for (unsigned int i = 0; i < this->parentSkeleton->joints.size(); ++i)
			{
				if (this->parentSkeleton->joints[i].mName == inJointName)
				{
					joint_index = i;
					break;
				}
			}

			if (joint_index >= 0) // if joint name existed
			{
				DirectX::XMFLOAT4X4 new_mat;
				// Convert FbxMatrix to XMFLOAT
				for (int i = 0; i < 4; ++i)
				{
					for (int j = 0; j < 4; ++j)
					{
						new_mat.m[i][j] = static_cast<float>(parentSkeleton->joints[joint_index].mGlobalBindposeInverse.Get(i, j));
					}
				}
				return new_mat;
			}
			else
			{
				// Return error matrix
				return DirectX::XMFLOAT4X4(-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f);
			}
		}
	};

	// Used for loading the very basics of an FBX
	// Input: std::string file name of FBX file, pointers to std::vectors to append the data to
	// Output: Appends data to the provided vectors, returns HRESULT
	HRESULT LoadFBX(const std::string& fileName, std::vector<DirectX::XMFLOAT3>* pOutVertexPosVector, std::vector<int>* pOutIndexVector,
		std::vector<DirectX::XMFLOAT3>* pOutNormalVector, std::vector<DirectX::XMFLOAT2>* pOutUVVector, Skeleton* pOutSkeleton, std::vector<FbxLoader::ControlPointInfo>* pOutCPInfoVector);



}