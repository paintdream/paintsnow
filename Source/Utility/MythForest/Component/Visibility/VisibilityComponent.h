// VisibilityComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../Explorer/SpaceTraversal.h"
#include "../Explorer/CameraCuller.h"
#include "../../../../Core/Template/TBuffer.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/Passes/ConstMapPass.h"
#include "../Stream/StreamComponent.h"

namespace PaintsNow {
	class SpaceComponent;
	class TransformComponent;
	class RenderableComponent;

	struct VisibilityComponentConfig {
		struct WorldInstanceData : public TReflected<WorldInstanceData, PassBase::PartialData> {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override;

			MatrixFloat4x4 worldMatrix;
			Float3Pair boundingBox;
			Float4 instancedColor;
		};

		struct InstanceGroup {
		public:
			InstanceGroup() : instanceCount(0) {}
			void Reset();
			PassBase::PartialUpdater instanceUpdater;
			std::vector<Bytes> instancedData;
			IRender::Resource::DrawCallDescription drawCallDescription;
			uint32_t instanceCount;
		};

		struct CaptureData : public FrustrumCuller {};

		class Cache {
		public:
			Int3 intPosition;
			Bytes payload;
		};

		class Cell : public TAllocatedTiny<Cell, SharedTiny>, public Cache {
		public:
			Cell();
			uint32_t finishCount;
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
				STATUS_POSTPROCESS,
			};

			TaskData() : status(STATUS_IDLE), pendingCount(0), renderQueue(nullptr), renderTarget(nullptr) {}

			struct WarpData {
				BytesCache bytesCache;
				std::unordered_map<size_t, InstanceGroup> instanceGroups;
			};

			uint32_t pendingCount;
			uint32_t status;
			TShared<Cell> cell;
			IRender::Queue* renderQueue;
			IRender::Resource* renderTarget;
			TShared<TextureResource> texture;
			std::vector<IDataUpdater*> dataUpdaters;
			Bytes data;
			PerspectiveCamera camera;
			std::vector<WarpData> warpData;
		};
	};

	class VisibilityComponent : public TAllocatedTiny<VisibilityComponent, UniqueComponent<Component, SLOT_VISIBILITY_COMPONENT> >, public SpaceTraversal<VisibilityComponent, VisibilityComponentConfig> {
	public:
		friend class SpaceTraversal<VisibilityComponent, VisibilityComponentConfig>;
		VisibilityComponent(const TShared<StreamComponent>& streamComponent);
		enum {
			VISIBILITYCOMPONENT_PARALLEL = COMPONENT_CUSTOM_BEGIN,
			VISIBILITYCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
		};

		typedef VisibilityComponentConfig::InstanceGroup InstanceGroup;
		typedef VisibilityComponentConfig::Cache Cache;
		typedef VisibilityComponentConfig::Cell Cell;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize(Engine& engine, Entity* entity) final;
		void Uninitialize(Engine& engine, Entity* entity) final;
		FLAG GetEntityFlagMask() const final;
		void DispatchEvent(Event& event, Entity* entity) final;
		Entity* GetHostEntity() const final;

		const Bytes& QuerySample(Engine& engine, const Float3& position);
		static bool IsVisible(const Bytes& sample, TransformComponent* transformComponent);

		void Setup(Engine& engine, float distance, const Float3& gridSize, uint32_t taskCount, const UShort2& resolution);

	protected:
		void TickRender(Engine& engine);
		void RoutineTickTasks(Engine& engine);
		void ResolveTasks(Engine& engine);
		void DispatchTasks(Engine& engine);
		void PostProcess(TaskData& task);

		void CoTaskAssembleTask(Engine& engine, TaskData& task, uint32_t face);
		void SetupIdentities(Engine& engine, Entity* hostEntity);
		void CollectRenderableComponent(Engine& engine, TaskData& task, RenderableComponent* renderableComponent, WorldInstanceData& instanceData, uint32_t identity);
		void CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity);
		void CompleteCollect(Engine& engine, TaskData& task);

		TShared<SharedTiny> StreamLoadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context);
		TShared<SharedTiny> StreamUnloadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context);

	protected:
		TShared<StreamComponent> streamComponent;
		Entity* hostEntity;

		Float3 gridSize;
		float viewDistance;
		uint32_t taskCount;
		UShort2 resolution;
		std::atomic<uint32_t> maxVisIdentity;
		uint32_t activeCellCacheIndex;
		Cache cellCache[8];
		TShared<TObjectAllocator<Cell> > cellAllocator;

		// Runtime Baker
		IRender::Queue* renderQueue;
		IRender::Resource* depthStencilResource;
		IRender::Resource* stateResource;
		IThread::Lock* collectLock;
		TShared<ShaderResourceImpl<ConstMapPass> > pipeline;
		std::vector<TaskData> tasks;
		std::vector<TShared<Cell> > bakePoints;
	};
}
