#include "RayTraceComponent.h"
using namespace PaintsNow;

RayTraceComponent::RayTraceComponent() : captureSize(1920, 1080), superSample(4096) {
	completedPixelCount.store(0, std::memory_order_relaxed);
}

RayTraceComponent::~RayTraceComponent() {
	Cleanup();
}

void RayTraceComponent::SetCaptureSize(const UShort2& size) {
	captureSize = size;
}

const UShort2& RayTraceComponent::GetCaptureSize() const {
	return captureSize;
}

size_t RayTraceComponent::GetCompletedPixelCount() const {
	return completedPixelCount.load(std::memory_order_acquire);
}

void RayTraceComponent::Cleanup() {
	referenceCameraComponent = nullptr;
	for (size_t i = 0; i < mappedResources.size(); i++) {
		mappedResources[i]->Unmap();
	}

	mappedResources.clear();
}

void RayTraceComponent::Capture(Engine& engine, const TShared<CameraComponent>& cameraComponent) {
	assert(referenceCameraComponent() == nullptr);

	referenceCameraComponent = cameraComponent;
	completedPixelCount.store(0, std::memory_order_release);
	assert(mappedResources.empty());

	// map all resources
	
}

const TShared<TextureResource>& RayTraceComponent::GetCapturedTexture() const {
	return capturedTexture;
}
