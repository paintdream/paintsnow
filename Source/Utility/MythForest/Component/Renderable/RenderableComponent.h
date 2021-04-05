// RenderableComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../../../Core/Template/TCache.h"
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/GraphicResourceBase.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/SkeletonResource.h"
#include "../RenderFlow/RenderPolicy.h"

namespace PaintsNow {
	class IDrawCallProvider {
	public:
		struct InputRenderData {
			InputRenderData(float ref = 0.0f, ShaderResource* res = nullptr, const UShort2 resolution = UShort2(0, 0)) : overrideShaderTemplate(res), viewResolution(resolution), viewReference(ref) {}

			ShaderResource* overrideShaderTemplate;
			UShort2 viewResolution;
			float viewReference;
		};

		struct OutputRenderData {
			IRender::Resource::DrawCallDescription drawCallDescription;
			IRender::Resource::RenderStateDescription renderStateDescription;
			IDataUpdater* dataUpdater;
			TShared<ShaderResource> shaderResource;
			TShared<SharedTiny> host;
			uint16_t priority;
			uint16_t groupIndex;
			std::vector<MatrixFloat4x4> localTransforms;
			std::vector<std::pair<uint32_t, Bytes> > localInstancedData;
		};

		typedef TCacheAllocator<OutputRenderData, uint8_t> DrawCallAllocator;
		virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache) = 0;
	};

	class RenderableComponent : public TAllocatedTiny<RenderableComponent, Component>, public IDrawCallProvider {
	public:
		enum {
			RENDERABLECOMPONENT_CAMERAVIEW = COMPONENT_CUSTOM_BEGIN,
			RENDERABLECOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
		};

		RenderableComponent();

		Tiny::FLAG GetEntityFlagMask() const override;
		virtual size_t ReportGraphicMemoryUsage() const;
		const std::vector<TShared<RenderPolicy> >& GetRenderPolicies() const;
		void AddRenderPolicy(const TShared<RenderPolicy>& renderPolicy);
		void RemoveRenderPolicy(const TShared<RenderPolicy>& renderPolicy);
		TObject<IReflect>& operator () (IReflect& reflect) override;

	private:
		std::vector<TShared<RenderPolicy> > renderPolicies;
	};
}
