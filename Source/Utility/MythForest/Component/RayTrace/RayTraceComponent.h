// RayTraceComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../Camera/CameraComponent.h"

namespace PaintsNow {
	class RayTraceComponent : public TAllocatedTiny<RayTraceComponent, Component> {
	public:
		RayTraceComponent();
		~RayTraceComponent() override;

		size_t GetCompletedPixelCount() const;
		void SetCaptureSize(const UShort2& size);
		const UShort2& GetCaptureSize() const;
		void Capture(Engine& engine, const TShared<CameraComponent>& cameraComponent);
		const TShared<TextureResource>& GetCapturedTexture() const;

	protected:
		void Cleanup();

	protected:
		UShort2 captureSize;
		uint16_t superSample;

		// progress context
		std::atomic<size_t> completedPixelCount;
		TShared<TextureResource> capturedTexture;
		TShared<CameraComponent> referenceCameraComponent;
		std::vector<TShared<ResourceBase> > mappedResources;
	};
}

