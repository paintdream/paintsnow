#include "PerspectiveCamera.h"
using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

const double PI = 3.14159265358979323846;
PerspectiveCamera::PerspectiveCamera() : fov((float)PI / 2.0f), aspect(1.0f), nearPlane(0.05f), farPlane(1000.0f) {}

static inline Float4 BuildPlane(const Float3& a, const Float3& b, const Float3& c) {
	Float3 u = a - b;
	Float3 v = c - b;
	Float3 n = CrossProduct(u, v).Normalize();
	return Float4(n.x(), n.y(), n.z(), -DotProduct(a, n));
}

bool FrustrumCuller::operator ()(const Float3Pair& box) const {
	// check visibility
	// Float3Pair box(Float3(0, -6, 0), Float3(0, -5, 0));
	Float3 size = (box.second - box.first) * 0.5f;
	Float3 center = box.first + size;
	for (size_t i = 0; i < sizeof(planes) / sizeof(planes[0]); i++) {
		const Float4& plane = planes[i];
		float r = fabs(size.x() * plane.x()) + fabs(size.y() * plane.y()) + fabs(size.z() * plane.z());
		Float3 n(plane.x(), plane.y(), plane.z());
		if (DotProduct(n, center) + r + plane.w() < -1e-4) {
			return false;
		}
	}

	return true;
}


Float3 FrustrumCuller::GetPosition() const {
	return Float3(viewTransform(3, 0), viewTransform(3, 1), viewTransform(3, 2));
}

Float3 FrustrumCuller::GetDirection() const {
	return Float3(-viewTransform(2, 0), -viewTransform(2, 1), -viewTransform(2, 2));
}

void PerspectiveCamera::UpdateCaptureData(FrustrumCuller& captureData, const MatrixFloat4x4& cameraWorldMatrix) const {
	Float3 position(cameraWorldMatrix(3, 0), cameraWorldMatrix(3, 1), cameraWorldMatrix(3, 2));
	Float3 up(cameraWorldMatrix(1, 0), cameraWorldMatrix(1, 1), cameraWorldMatrix(1, 2));
	Float3 direction(-cameraWorldMatrix(2, 0), -cameraWorldMatrix(2, 1), -cameraWorldMatrix(2, 2));
	direction.Normalize();

	captureData.viewTransform = cameraWorldMatrix;

	// update planes ...
	float tanHalfFov = (float)tan(fov / 2.0f);
	Float3 basisX = CrossProduct(direction, up).Normalize();
	Float3 basisY = CrossProduct(basisX, direction);
	float nearStepY = nearPlane * tanHalfFov;
	float nearStepX = nearStepY * aspect;
	float farStepY = farPlane * tanHalfFov;
	float farStepX = farStepY * aspect;

	Float3 nearCenter = position + direction * nearPlane;
	Float3 farCenter = position + direction * farPlane;

	Float3 nearLeftBottom = nearCenter - basisX * nearStepX - basisY * nearStepY;
	Float3 nearRightBottom = nearCenter + basisX * nearStepX - basisY * nearStepY;
	Float3 nearLeftTop = nearCenter - basisX * nearStepX + basisY * nearStepY;
	Float3 nearRightTop = nearCenter + basisX * nearStepX + basisY * nearStepY;

	Float3 farLeftBottom = farCenter - basisX * farStepX - basisY * farStepY;
	Float3 farRightBottom = farCenter + basisX * farStepX - basisY * farStepY;
	Float3 farLeftTop = farCenter - basisX * farStepX + basisY * farStepY;
	Float3 farRightTop = farCenter + basisX * farStepX + basisY * farStepY;

	captureData.planes[0] = BuildPlane(nearLeftTop, nearRightTop, farLeftTop);				// Top
	captureData.planes[1] = BuildPlane(nearLeftBottom, farLeftBottom, nearRightBottom);		// Bottom
	captureData.planes[2] = BuildPlane(nearLeftTop, farLeftTop, farLeftBottom);				// Left
	captureData.planes[3] = BuildPlane(nearRightTop, farRightBottom, farRightTop);			// Right
	captureData.planes[4] = BuildPlane(nearLeftTop, nearLeftBottom, nearRightBottom);		// Front
	captureData.planes[5] = BuildPlane(farLeftTop, farRightBottom, farLeftBottom);			// Back
}
