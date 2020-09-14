// ModelComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../../Component/Batch/BatchComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	class ModelComponent : public TAllocatedTiny<ModelComponent, RenderableComponent> {
	public:
		enum {
			MODELCOMPONENT_HAS_ANIMATION = RENDERABLECOMPONENT_CUSTOM_BEGIN,
			MODELCOMPONENT_CUSTOM_BEGIN = RENDERABLECOMPONENT_CUSTOM_BEGIN << 1
		};

		// delayed loader
		ModelComponent(TShared<MeshResource> meshResource, TShared<BatchComponent> batch);

		String GetDescription() const override;
		void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;

		void AddMaterial(uint32_t meshGroupIndex, TShared<MaterialResource>& materialResource);
		uint32_t CreateOverrider(TShared<ShaderResource> shaderResourceTemplate);
		size_t ReportGraphicMemoryUsage() const override;

	protected:
		uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;
		virtual void GenerateDrawCalls(std::vector<OutputRenderData>& drawCallTemplates, std::vector<std::pair<uint32_t, TShared<MaterialResource> > >& materialResources);

		void Collapse(Engine& engine);
		void Expand(Engine& engine);

		struct CollapseData {
			String meshResourceLocation;
			std::vector<String> materialResourceLocations;
		};

	protected:
		TShared<MeshResource> meshResource;
		TShared<BatchComponent> batchComponent;

		std::vector<TShared<ShaderResource> > shaderOverriders;
		std::vector<std::pair<uint32_t, TShared<MaterialResource> > > materialResources;
		std::vector<OutputRenderData> drawCallTemplates;

		CollapseData collapseData;
		uint32_t hostCount;
	};
}
