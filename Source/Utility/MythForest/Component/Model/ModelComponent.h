// ModelComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __MODELCOMPONENT_H__
#define __MODELCOMPONENT_H__

#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../../Component/Batch/BatchComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ModelComponent : public TAllocatedTiny<ModelComponent, RenderableComponent> {
		protected:
			ModelComponent(); // only for derived classes

		public:
			enum {
				MODELCOMPONENT_HAS_ANIMATION = RENDERABLECOMPONENT_CUSTOM_BEGIN,
				MODELCOMPONENT_CUSTOM_BEGIN = RENDERABLECOMPONENT_CUSTOM_BEGIN << 1
			};

			// delayed loader
			ModelComponent(TShared<NsSnowyStream::MeshResource> meshResource, TShared<BatchComponent> batch);

			virtual String GetDescription() const override;
			virtual void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void Initialize(Engine& engine, Entity* entity) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;

			void AddMaterial(uint32_t meshGroupIndex, TShared<NsSnowyStream::MaterialResource>& materialResource);
			uint32_t CreateOverrider(TShared<NsSnowyStream::ShaderResource> shaderResourceTemplate);
			virtual size_t ReportGraphicMemoryUsage() const;

		protected:
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;
			virtual void GenerateDrawCalls(std::vector<OutputRenderData>& drawCallTemplates, std::vector<std::pair<uint32_t, TShared<NsSnowyStream::MaterialResource> > >& materialResources);

			void Collapse(Engine& engine);
			void Expand(Engine& engine);

			struct CollapseData {
				String meshResourceLocation;
				std::vector<String> materialResourceLocations;
			};

		protected:
			TShared<NsSnowyStream::MeshResource> meshResource;
			TShared<BatchComponent> batchComponent;

			std::vector<TShared<NsSnowyStream::ShaderResource> > shaderOverriders;
			std::vector<std::pair<uint32_t, TShared<NsSnowyStream::MaterialResource> > > materialResources;
			std::vector<OutputRenderData> drawCallTemplates;

			CollapseData collapseData;
			uint32_t hostCount;
		};
	}
}


#endif // __MODELCOMPONENT_H__
