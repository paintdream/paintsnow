// VisibilityComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __VISIBILITYCOMPONENT_H__
#define __VISIBILITYCOMPONENT_H__

#include "../../Entity.h"
#include "../../Component.h"
#include "../Explorer/SpaceTraversal.h"
#include "../../../../Core/Template/TBuffer.h"
#include "../../../../General/Misc/ZRenderQueue.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/Passes/ConstMapPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class SpaceComponent;
		class TransformComponent;
		class RenderableComponent;

		struct VisibilityComponentConfig {
			struct WorldInstanceData : public TReflected<WorldInstanceData, ZPassBase::PartialData> {
			public:
				virtual TObject<IReflect>& operator () (IReflect& reflect) override;

				MatrixFloat4x4 worldMatrix;
				Float3Pair boundingBox;
				Float4 instancedColor;
			};

			struct InstanceGroup {
			public:
				InstanceGroup() : instanceCount(0) {}
				void Reset();
				ZPassBase::PartialUpdater partialUpdater;
				std::vector<Bytes> instancedData;
				IRender::Resource::DrawCallDescription drawCallDescription;
				uint32_t instanceCount;
			};

			struct CaptureData {
				CaptureData(const Float3& v, bool(*c)(const Float3Pair&)) : viewPosition(v), culler(c) {}
				inline bool operator () (const Float3Pair& box) const {
					return culler(Float3Pair(box.first - viewPosition, box.second - viewPosition));
				}

				bool(*culler)(const Float3Pair&);
				Float3 viewPosition;
			};

			struct TaskData {
				enum {
					STATUS_IDLE,
					STATUS_START,
					STATUS_DISPATCHED,
					STATUS_ASSEMBLING,
					STATUS_ASSEMBLED,
					STATUS_BAKING,
					STATUS_BAKED,
				};

				TaskData() : status(STATUS_IDLE), pendingCount(0), renderQueue(nullptr), renderTarget(nullptr) {}

				uint32_t pendingCount;
				uint32_t status;
				UShort3 coord;
				IRender::Queue* renderQueue;
				IRender::Resource* renderTarget;
				TShared<NsSnowyStream::TextureResource> texture;
				std::vector<NsSnowyStream::IDrawCallProvider::DataUpdater*> dataUpdaters;
				Bytes data;
				std::vector<std::map<size_t, InstanceGroup> > instanceGroups;
			};
		};

		class VisibilityComponent : public TAllocatedTiny<VisibilityComponent, UniqueComponent<Component, SLOT_VISIBILITY_COMPONENT> >, public SpaceTraversal<VisibilityComponent, VisibilityComponentConfig> {
		public:
			friend class SpaceTraversal<VisibilityComponent, VisibilityComponentConfig>;
			VisibilityComponent();
			enum {
				VISIBILITYCOMPONENT_PARALLEL = COMPONENT_CUSTOM_BEGIN,
				VISIBILITYCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
			};

			typedef VisibilityComponentConfig::InstanceGroup InstanceGroup;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void Initialize(Engine& engine, Entity* entity) override final;
			virtual void Uninitialize(Engine& engine, Entity* entity) override final;
			virtual FLAG GetEntityFlagMask() const override final;
			virtual void DispatchEvent(Event& event, Entity* entity) override final;

			const Bytes& QuerySample(const Float3& position);
			static bool IsVisible(const Bytes& sample, TransformComponent* transformComponent);

			void Setup(Engine& engine, float distance, const Float3Pair& range, const UShort3& division, uint32_t frameTimeLimit, uint32_t taskCount, const UShort2& resolution);

		protected:
			struct Cell {
			public:
				Cell();
				Bytes payload;
				uint32_t incompleteness;
			};

			struct Cache {
				Cache() : counter(0) {}
				UShort3 index;
				uint16_t counter;
				Bytes mergedPayload;
			};

			void TickRender(Engine& engine);
			void RoutineTickTasks(Engine& engine);
			void ResolveTasks(Engine& engine);
			void DispatchTasks(Engine& engine);

			struct BakePoint {
				UShort3 coord;
				uint16_t face;
			};

			void CoTaskAssembleTask(Engine& engine, TaskData& task, const BakePoint& bakePoint);
			void SetupIdentities(Engine& engine, Entity* hostEntity);
			void CollectRenderableComponent(Engine& engine, TaskData& task, RenderableComponent* renderableComponent, WorldInstanceData& instanceData, uint32_t identity);
			void CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity);
			void CompleteCollect(Engine& engine, TaskData& task);

		protected:
			Entity* hostEntity;

			Float3Pair boundingBox;
			UShort3 subDivision;
			UShort2 resolution;
			uint32_t taskCount;
			uint32_t maxFrameExecutionTime; // in ms
			float viewDistance;
			TAtomic<uint32_t> maxVisIdentity;
			std::map<UShort3, Cell> cells;
			uint32_t activeCellCacheIndex;
			Cache cellCache[8];

			// Runtime Baker
			IRender::Queue* renderQueue;
			IRender::Resource* clearResource;
			IRender::Resource* depthStencilResource;
			IRender::Resource* stateResource;
			TShared<NsSnowyStream::ShaderResourceImpl<NsSnowyStream::ConstMapPass> > pipeline;
			TAtomic<uint32_t> collectCritical;

			std::vector<TaskData> tasks;
			std::stack<BakePoint> bakePoints;
		};
	}
}


#endif // __VISIBILITYCOMPONENT_H__
