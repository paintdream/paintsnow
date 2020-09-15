#include "SkeletonResource.h"
#include "../../../Core/Template/TAlgorithm.h"
#include <cassert>
#include <cmath>

using namespace PaintsNow;

const double EPSILON = 1e-7;
typedef Quaternion<float> Quater;

using namespace PaintsNow;

SkeletonResource::SkeletonResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

TObject<IReflect>& SkeletonResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(boneAnimation);
	}

	return *this;
}

void SkeletonResource::Attach(IRender& render, void* deviceContext) {
	offsetMatrices.resize(boneAnimation.joints.size());
	offsetMatricesInv.resize(boneAnimation.joints.size());
	for (size_t i = 0; i < boneAnimation.joints.size(); i++) {
		PrepareOffsetTransform(i);
	}
}

void SkeletonResource::Detach(IRender& render, void* deviceContext) {}
void SkeletonResource::Upload(IRender& render, void* deviceContext) {}
void SkeletonResource::Download(IRender& render, void* deviceContext) {}

void SkeletonResource::PrepareTransform(std::vector<MatrixFloat4x4>& transforms, size_t k) {
	const Joint& joint = boneAnimation.joints[k];
	int parent = joint.parent;
	if (parent != -1) {
		transforms[k] = transforms[k] * transforms[parent];
	}
}

void SkeletonResource::PrepareOffsetTransform(size_t k) {
	const Joint& joint = boneAnimation.joints[k];
	int parent = joint.parent;

	offsetMatrices[k] = joint.offsetMatrix;

	// mult all parents
	if (parent != -1) {
		offsetMatrices[k] = offsetMatrices[k] * offsetMatrices[parent];
	}

	offsetMatricesInv[k] = Math::QuickInverse(offsetMatrices[k]);
}

template <class T>
uint32_t LocateFrame(const std::vector<T>& frames, float time) {
	T t;
	t.time = time;
	size_t frame = std::lower_bound(frames.begin(), frames.end(), t) - frames.begin();
	return safe_cast<uint32_t>(Math::Min(frame, frames.size() - 1));
}

const IAsset::BoneAnimation& SkeletonResource::GetBoneAnimation() const {
	return boneAnimation;
}

const std::vector<MatrixFloat4x4>& SkeletonResource::GetOffsetTransforms() const {
	return offsetMatrices;
}

void SkeletonResource::UpdateBoneMatrixBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer, const std::vector<MatrixFloat4x4>& data) {
	UpdateBuffer(render, queue, buffer, const_cast<std::vector<MatrixFloat4x4>&>(data), IRender::Resource::BufferDescription::UNIFORM, "BoneMatrix");
}

void SkeletonResource::ClearBoneMatrixBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer) {
	ClearBuffer(render, queue, buffer);
}

float SkeletonResource::Snapshot(std::vector<MatrixFloat4x4>& transforms, uint32_t clipIndex, float time, bool repeat) {
	assert(transforms.size() == boneAnimation.joints.size());
	if (boneAnimation.clips.size() == 0) {
		return time;
	}

	const IAsset::BoneAnimation::Clip& clip = boneAnimation.clips[clipIndex];
	if (time > clip.duration) {
		if (repeat) {
			time = fmod(time, clip.duration);
		} else {
			time = clip.duration;
		}
	}

	for (size_t c = 0; c < clip.channels.size(); c++) {
		const IAsset::BoneAnimation::Channel& channel = clip.channels[c];
		int a = channel.jointIndex;
		const Joint* joint = &boneAnimation.joints[a];

		// process positions
		Float3 presentPosition;

		if (channel.transSequence.frames.size() != 0) {
			// interpolate between this frame and the next one.
			uint32_t frame = LocateFrame(channel.transSequence.frames, time);
			uint32_t nextFrame = (frame + 1) % (int)channel.transSequence.frames.size();
			const IAsset::TransSequence::Frame& key = channel.transSequence.frames[frame];
			const IAsset::TransSequence::Frame& nextKey = channel.transSequence.frames[nextFrame];
			float diffTime = nextKey.time - key.time;

			if (diffTime < 0)
				diffTime += clip.duration;

			if (diffTime > 0) {
				float factor = ((float)time - key.time) / diffTime;
				// linear interpolation
				float factor1;
				float factor2;
				float factor3;
				float factor4;
				float factorTimesTwo;
				float inverseFactorTimesTwo;

				float inverseFactor = 1.0f - factor;

				switch (channel.transSequence.interpolate) {
					case IAsset::INTERPOLATE_NONE:
						presentPosition = key.value;
						break;
					case IAsset::INTERPOLATE_LINEAR:
						presentPosition = key.value + (nextKey.value - key.value) * factor;
						break;
					case IAsset::INTERPOLATE_HERMITE:
						factorTimesTwo = factor * factor;

						factor1 = factorTimesTwo * (2.0f * factor - 3.0f) + 1;
						factor2 = factorTimesTwo * (factor - 2.0f) + factor;
						factor3 = factorTimesTwo * (factor - 1.0f);
						factor4 = factorTimesTwo * (3.0f - 2.0f * factor);
						presentPosition = key.value * factor1 + key.outTan * factor2 + nextKey.inTan * factor3 + nextKey.outTan * factor4;
						break;
					case IAsset::INTERPOLATE_BEZIER:
						factorTimesTwo = factor * factor;
						inverseFactorTimesTwo = inverseFactor * inverseFactor;

						factor1 = inverseFactorTimesTwo * inverseFactor;
						factor2 = 3.0f * factor * inverseFactorTimesTwo;
						factor3 = 3.0f * factorTimesTwo * inverseFactor;
						factor4 = factorTimesTwo * factor;

						presentPosition = key.value * factor1 + key.outTan * factor2 + nextKey.inTan * factor3 + nextKey.outTan * factor4;
						break;
				}
			} else {
				presentPosition = key.value;
			}
		}

		// process rotation
		Quater presentRotation;

		if (channel.rotSequence.frames.size() != 0) {
			// interpolate between this frame and the next one.
			uint32_t frame = LocateFrame(channel.rotSequence.frames, time);
			uint32_t nextFrame = (frame + 1) % (int)channel.rotSequence.frames.size();
			const IAsset::RotSequence::Frame& key = channel.rotSequence.frames[frame];
			const IAsset::RotSequence::Frame& nextKey = channel.rotSequence.frames[nextFrame];
			float diffTime = nextKey.time - key.time;

			if (diffTime < 0)
				diffTime += clip.duration;

			if (diffTime > EPSILON) {
				float factor = ((float)time - key.time) / diffTime;
				// linear interpolation
				switch (channel.rotSequence.interpolate) {
					case IAsset::INTERPOLATE_NONE:
						presentRotation = key.value;
						break;
					case IAsset::INTERPOLATE_LINEAR:
						Quater::Interpolate(presentRotation, Quater(key.value), Quater(nextKey.value), factor);
						break;
					case IAsset::INTERPOLATE_HERMITE:
					case IAsset::INTERPOLATE_BEZIER:
						Quater::InterpolateSquad(presentRotation, Quater(key.value), Quater(key.outTan), Quater(nextKey.value), Quater(nextKey.inTan), factor);
						break;
				}
			} else {
				presentRotation = key.value;
			}
		}

		// process scaling
		Float3 presentScaling(1, 1, 1);

		if (channel.scalingSequence.frames.size() != 0) {
			uint32_t frame = LocateFrame(channel.scalingSequence.frames, time);
			uint32_t nextFrame = (frame + 1) % (int)channel.scalingSequence.frames.size();
			const IAsset::ScalingSequence::Frame& key = channel.scalingSequence.frames[frame];
			const IAsset::ScalingSequence::Frame& nextKey = channel.scalingSequence.frames[nextFrame];
			float diffTime = nextKey.time - key.time;

			if (diffTime < 0)
				diffTime += clip.duration;

			if (diffTime > EPSILON) {
				float factor = ((float)time - key.time) / diffTime;
				// only support linear blend ...
				presentScaling = key.value + (nextKey.value - key.value) * factor;
			} else {
				presentScaling = channel.scalingSequence.frames[frame].value;
			}
		}

		MatrixFloat4x4 mat;
		presentRotation.WriteMatrix(mat);
		mat = mat.Transpose();

		for (int i = 0; i < 3; i++)
			mat(0, i) *= presentScaling.x();
		for (int j = 0; j < 3; j++)
			mat(1, j) *= presentScaling.y();
		for (int k = 0; k < 3; k++)
			mat(2, k) *= presentScaling.z();

		mat(3, 0) = presentPosition.x();
		mat(3, 1) = presentPosition.y();
		mat(3, 2) = presentPosition.z();

		transforms[a] = mat * offsetMatrices[a];
	}

	// make transforms from top to end
	for (size_t k = 0; k < boneAnimation.joints.size(); k++) {
		PrepareTransform(transforms, k);
	}

	for (size_t d = 0; d < boneAnimation.joints.size(); d++) {
		transforms[d] = offsetMatricesInv[d] * transforms[d];
	}

	return time;
}