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
	class SpaceComponent;
	class RayTraceComponent : public TAllocatedTiny<RayTraceComponent, Component> {
	public:
		RayTraceComponent();
		~RayTraceComponent() override;

		typedef RenderPortLightSource::LightElement LightElement;
		size_t GetCompletedPixelCount() const;
		size_t GetTotalPixelCount() const;
		void SetCaptureSize(const UShort2& size);
		const UShort2& GetCaptureSize() const;
		void Capture(Engine& engine, const TShared<CameraComponent>& cameraComponent, float averageLuminance);
		TShared<TextureResource> GetCapturedTexture() const;
		void SetOutputPath(const String& path);
		void Configure(uint16_t superSample, uint16_t tileSize, uint32_t rayCount, uint32_t maxBounceCount);

	protected:
		// progress context
		class Context : public TReflected<Context, SharedTiny> {
		public:
			Context(Engine& engine);
			~Context();

			Engine& engine;
			TShared<TextureResource> capturedTexture;
			TShared<CameraComponent> referenceCameraComponent;

			Float3 view;
			Float3 forward;
			Float3 up;
			Float3 right;
			float invAverageLuminance;
			float possibilityForGeometryLight;
			float possibilityCubeMapLight;
			int64_t clock;
			std::vector<SpaceComponent*> rootSpaceComponents;
			std::vector<Float2> randomSequence;
			std::vector<LightElement> lightElements;
			std::unordered_map<size_t, uint32_t> mapEntityToResourceIndex;
			std::vector<TShared<ResourceBase> > mappedResources;
			std::vector<BytesCache> threadLocalCache;
			TShared<TextureResource> cubeMapTexture;
			std::vector<Float4> importantCubeMapDistribution;
			std::atomic<size_t> completedPixelCount;
		};

	protected:
		void RoutineRayTrace(const TShared<Context>& context);
		void RoutineCollectTextures(const TShared<Context>& context, Entity* rootEntity, const MatrixFloat4x4& worldMatrix);
		void RoutineRenderTile(const TShared<Context>& context, size_t i, size_t j);
		void RoutineComplete(const TShared<Context>& context);
		Float4 PathTrace(const TShared<Context>& context, const Float3Pair& ray, BytesCache& cache, uint32_t count) const;
		void Raycast(RaycastTaskSerial& task, const TShared<Context>& context, const Float3Pair& r) const;

		Float4 maxEnvironmentRadiance;
		UShort2 captureSize;
		uint16_t superSample;
		uint16_t tileSize;
		uint32_t rayCount;
		uint32_t maxBounceCount;
		uint32_t cubeMapQuantization;
		uint32_t cubeMapQuantizationMultiplier;
		String outputPath;
		size_t completedPixelCountSync;
		float stepMinimal;
		float stepMaximal;
		TShared<TextureResource> capturedTexture;
	};
}

