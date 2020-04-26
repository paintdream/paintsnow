#include "TransformComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

TransformComponent::TransformComponent() : uniqueObjectID(0) {}

TObject<IReflect>& TransformComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform);
		ReflectProperty(trsData)[Runtime];
	}

	return *this;
}

void TransformComponent::GetAxises(Float3& xAxis, Float3& yAxis, Float3& zAxis) const {
	const MatrixFloat4x4& m = transform;
	xAxis = Float3(m(0, 0), m(0, 1), m(0, 2));
	yAxis = Float3(m(1, 0), m(1, 1), m(1, 2));
	zAxis = Float3(m(2, 0), m(2, 1), m(2, 2));
}

TShared<TransformComponent::TRS>& TransformComponent::Modify() {
	if (!trsData) {
		trsData = TShared<TRS>::From(new TRS());
		trsData->rotation = Quaternion<float>(transform);
		trsData->translation = Float3(transform(3, 0), transform(3, 1), transform(3, 2));
		trsData->scale = Float3(
			Float3(transform(0, 0), transform(0, 1), transform(0, 2)).Length(),
			Float3(transform(1, 0), transform(1, 1), transform(1, 2)).Length(),
			Float3(transform(2, 0), transform(2, 1), transform(2, 2)).Length()
		);
	}

	return trsData;
}

void TransformComponent::SetRotation(const Float3& r) {
	Modify()->rotation = Quaternion<float>(r);
	Flag() |= TINY_MODIFIED;
}

static inline Float3 GetArcballVector(const Float2& pt) {
	Float3 ret(pt.x(), pt.y(), 0);

	float sq = pt.SquareLength();
	if (sq < 1) {
		ret.z() = sqrt(1 - sq);
		return ret;
	} else {
		return ret.Normalize();
	}
}

void TransformComponent::EditorRotate(const Float2& from, const Float2& to) {
	Float3 x, y, z;
	Float3 position = GetQuickTranslation();
	GetAxises(x, y, z);
	z = z + x * (from.x() - to.x()) + y * (from.y() - to.y());
	z.Normalize();
	x = CrossProduct(Float3(0, 0, 1), z);
	x.Normalize();
	y = CrossProduct(z, x);

	transform(0, 0) = x.x();
	transform(0, 1) = x.y();
	transform(0, 2) = x.z();

	transform(1, 0) = y.x();
	transform(1, 1) = y.y();
	transform(1, 2) = y.z();

	transform(2, 0) = z.x();
	transform(2, 1) = z.y();
	transform(2, 2) = z.z();

	trsData = nullptr;
	// LookAt(-position, -z, Float3(0, 0, 1));
		/*
	// Quaternion<float> delta(Float3(0, to.y() - from.y(), -(to.x() - from.x())));
	// MakeEditor()->rotation = MakeEditor()->rotation * delta;
	MakeEditor()->rotation = Quaternion<float>::Align(GetArcballVector(from), GetArcballVector(to)) * MakeEditor()->rotation;
	Flag() |= TINY_MODIFIED;*/
}

Float3 TransformComponent::GetRotation() {
	return Modify()->rotation.ToEulerAngle();
}

const Quaternion<float>& TransformComponent::GetRotationQuaternion() {
	return Modify()->rotation;
}

void TransformComponent::SetTranslation(const Float3& t) {
	Modify()->translation = t;
	Flag() |= TINY_MODIFIED;
}

const Float3& TransformComponent::GetTranslation() {
	return Modify()->translation;
}

Float3 TransformComponent::GetQuickTranslation() const {
	return Float3(transform(3, 0), transform(3, 1), transform(3, 2));
}

void TransformComponent::SetScale(const Float3& s) {
	Modify()->scale = s;
	Flag() |= TINY_MODIFIED;
}

const Float3& TransformComponent::GetScale() {
	return Modify()->scale;
}

const MatrixFloat4x4& TransformComponent::GetTransform() const {
	return transform;
}

void TransformComponent::SetTransform(const MatrixFloat4x4& value) {
	if (trsData) trsData = nullptr;
	transform = value;
}

void TransformComponent::UpdateTransform() {
	if (Flag() & Tiny::TINY_MODIFIED) {
		MatrixFloat4x4 rotMatrix;
		TShared<TRS>& trsData = Modify();
		trsData->rotation.WriteMatrix(rotMatrix);
		const Float3& scale = trsData->scale;
		const Float3& translation = trsData->translation;

		float s[16] = {
			scale.x(), 0, 0, 0,
			0, scale.y(), 0, 0,
			0, 0, scale.z(), 0,
			translation.x(), translation.y(), translation.z(), 1
		};

		transform = rotMatrix * MatrixFloat4x4(s);

		trsData = nullptr;
		Flag() &= ~Tiny::TINY_MODIFIED;
	}
}

void TransformComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box) {
	UpdateTransform();

	if (box.second.x() < box.first.x()) {
		// no other components with bounding box?
		box.first = box.second = GetQuickTranslation();
	} else {
		Float3Pair newBox(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
		for (uint8_t i = 0; i < 8; i++) {
			Float4 pt(i & 1 ? box.first.x() : box.second.x(), i & 2 ? box.first.y() : box.second.y(), i & 4 ? box.first.z() : box.second.z(), 1);

			pt = pt * transform;
			assert(fabs(pt.w() - 1) < 1e-3);
			Union(newBox, Float3(pt.x(), pt.y(), pt.z()));
		}

		box = newBox;
	}

	cacheBoundingBox = box;
}

uint32_t TransformComponent::GetObjectID() const {
	return uniqueObjectID;
}

void TransformComponent::SetObjectID(uint32_t id) {
	uniqueObjectID = id;
}

void TransformComponent::Raycast(std::vector<RaycastResult>& results, Float3Pair& ray, uint32_t maxCount, IReflectObject* filter) const {
	MatrixFloat4x4 invTransform = Inverse(transform);

	ray.second += ray.first;
	ray.first = Transform3D(invTransform, ray.first);
	ray.second = Transform3D(invTransform, ray.second) - ray.first;
}

const Float3Pair& TransformComponent::GetLocalBoundingBox() const {
	return cacheBoundingBox;
}
