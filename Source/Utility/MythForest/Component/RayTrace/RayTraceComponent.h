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

		typedef RenderPortLightSource::LightElement LightElement;
		size_t GetCompletedPixelCount() const;
		void SetCaptureSize(const UShort2& size);
		const UShort2& GetCaptureSize() const;
		void Capture(Engine& engine, const TShared<CameraComponent>& cameraComponent);
		TShared<TextureResource> GetCapturedTexture() const;
		void SetOutputPath(const String& path);

	protected:
		// progress context
		class Context : public TReflected<Context, SharedTiny> {
		public:
			Context(Engine& engine);
			~Context();

			Engine& engine;
			TShared<TextureResource> capturedTexture;
			TShared<CameraComponent> referenceCameraComponent;
			std::vector<LightElement> lightElements;
			std::vector<TShared<ResourceBase> > mappedResources;
			std::atomic<size_t> completedPixelCount;
		};

	protected:
		void Cleanup();
		void RoutineRayTrace(const TShared<Context>& context);
		void RoutineCollectTextures(const TShared<Context>& context, Entity* rootEntity, const MatrixFloat4x4& worldMatrix);
		void RoutineRenderTile(const TShared<Context>& context, size_t i, size_t j);
		void RoutineComplete(const TShared<Context>& context);

		UShort2 captureSize;
		uint16_t superSample;
		uint16_t tileSize;
		String outputPath;
		TShared<Context> currentContext;
	};
}

