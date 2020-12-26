// CameraComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/Template/TBuffer.h"
#include "../../../../Core/Template/TAlgorithm.h"
#include "../../../../Core/Template/TCache.h"
#include "../Animation/AnimationComponent.h"
#include "../RenderFlow/RenderFlowComponent.h"
#include "../RenderFlow/RenderPort/RenderPortCommandQueue.h"
#include "../RenderFlow/RenderPort/RenderPortLightSource.h"
#include "../Event/EventComponent.h"
#include "../Explorer/CameraCuller.h"
#include "../Explorer/SpaceTraversal.h"
#include <sstream>

namespace PaintsNow {
	class LightComponent;
	class EnvCubeComponent;
	class SpaceComponent;
	class RenderableComponent;

	struct CameraComponentConfig {
		typedef RenderPortLightSource::LightElement LightElement;
		typedef RenderPortLightSource::EnvCubeElement EnvCubeElement;

		struct CaptureData : public FrustrumCuller {
			Bytes visData;
		};

		struct WorldGlobalData : public TReflected<WorldGlobalData, PassBase::PartialData> {
			TObject<IReflect>& operator () (IReflect& reflect) override;
			MatrixFloat4x4 viewProjectionMatrix;
			MatrixFloat4x4 projectionMatrix;
			MatrixFloat4x4 lastViewProjectionMatrix;
			MatrixFloat4x4 viewMatrix;
			MatrixFloat4x4 inverseViewMatrix; // inverse of viewMatrix
			MatrixFloat4x4 jitterMatrix;

			Float3 viewPosition;
			Float2 jitterOffset;
			float time;
			float tanHalfFov;
		};

		struct WorldInstanceData : public TReflected<WorldInstanceData, PassBase::PartialData> {
			WorldInstanceData() : viewReference(0), fadeRatio(0) {}
			TObject<IReflect>& operator () (IReflect& reflect) override;

			MatrixFloat4x4 worldMatrix;
			Float3Pair boundingBox;
			float viewReference; // currently only works on Lod selection
			float fadeRatio;
			TShared<AnimationComponent> animationComponent;
		};

		struct InstanceKey {
			inline bool operator == (const InstanceKey& rhs) const {
				return memcmp(this, &rhs, sizeof(*this)) == 0;
			}

			IRender::Resource::RenderStateDescription renderStateDescription;
			size_t renderKey;
			size_t animationKey;
		};

		struct HashInstanceKey {
			inline size_t operator () (const InstanceKey& key) const {
				return key.renderKey ^ key.animationKey;
			}
		};

		struct InstanceGroup {
			InstanceGroup() : drawCallResource(nullptr), instanceCount(0) {}
			void Cleanup();

			PassBase::PartialUpdater* instanceUpdater;
			std::vector<Bytes> instancedData;
			IRender::Resource::DrawCallDescription drawCallDescription;
			IRender::Resource* renderStateResource;
			IRender::Resource* drawCallResource;
			TShared<RenderPolicy> renderPolicy;
			TShared<AnimationComponent> animationComponent;
			uint32_t instanceCount;
#ifdef _DEBUG
			String description;
#endif
		};

		struct TaskData : public TReflected<TaskData, SharedTiny> {
			TaskData(uint32_t warpCount);
			~TaskData() override;
			void Cleanup(IRender& render);
			void Destroy(IRender& render);

			TObject<IReflect>& operator () (IReflect& reflect) override;

			struct PolicyData {
				PolicyData();
				IRender::Queue* portQueue;
				IRender::Resource* instanceBuffer;
				Bytes instanceData;
				size_t instanceOffset;
				std::vector<IRender::Resource*> runtimeResources;
			};

			struct_aligned(64) WarpData {
				typedef std::unordered_map<InstanceKey, InstanceGroup, HashInstanceKey> InstanceGroupMap;
				InstanceGroupMap instanceGroups;
				WarpData();

				struct GlobalBufferItem {
					IRender::Queue* renderQueue;
					PassBase::PartialUpdater globalUpdater;
					PassBase::PartialUpdater instanceUpdater;
					std::vector<IRender::Resource::DrawCallDescription::BufferRange> buffers;
					std::vector<IRender::Resource*> textures;
				};

				std::vector<std::key_value<ShaderResource*, GlobalBufferItem> > worldGlobalBufferMap;
				std::vector<std::key_value<IRender::Resource::RenderStateDescription, IRender::Resource*> > renderStateMap;
				std::vector<std::key_value<RenderPolicy*, PolicyData> > renderPolicyMap;
				std::vector<std::pair<TShared<RenderPolicy>, LightElement> > lightElements;
				std::vector<std::pair<TShared<RenderPolicy>, EnvCubeElement> > envCubeElements;
				std::vector<IDataUpdater*> dataUpdaters;
				BytesCache bytesCache;
				uint32_t entityCount;
				uint32_t visibleEntityCount;
				uint32_t triangleCount;
			};

			std::vector<WarpData> warpData;
			WorldGlobalData worldGlobalData;
			std::atomic<uint32_t> pendingCount;
		};
	};

	class CameraComponent : public TAllocatedTiny<CameraComponent, Component>, public PerspectiveCamera, public SpaceTraversal<CameraComponent, CameraComponentConfig> {
	public:
		enum {
			CAMERACOMPONENT_PERSPECTIVE = COMPONENT_CUSTOM_BEGIN,
			CAMERACOMPONENT_SMOOTH_TRACK = COMPONENT_CUSTOM_BEGIN << 1,
			CAMERACOMPONENT_SUBPIXEL_JITTER = COMPONENT_CUSTOM_BEGIN << 2,
			CAMERACOMPONENT_UPDATE_COLLECTING = COMPONENT_CUSTOM_BEGIN << 3,
			CAMERACOMPONENT_UPDATE_COLLECTED = COMPONENT_CUSTOM_BEGIN << 4,
			CAMERACOMPONENT_UPDATE_COMMITTED = COMPONENT_CUSTOM_BEGIN << 5,
			CAMERACOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 6
		};

		friend class SpaceTraversal<CameraComponent, CameraComponentConfig>;

		typedef typename CameraComponentConfig::InstanceKey InstanceKey;
		typedef typename CameraComponentConfig::HashInstanceKey HashInstanceKey;
		typedef typename CameraComponentConfig::InstanceGroup InstanceGroup;

		CameraComponent(const TShared<RenderFlowComponent>& renderFlowComponent, const String& cameraViewPortName);
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		Tiny::FLAG GetEntityFlagMask() const override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void BindRootEntity(Engine& engine, Entity* entity);
		uint32_t GetCollectedEntityCount() const;
		uint32_t GetCollectedVisibleEntityCount() const;
		uint32_t GetCollectedTriangleCount() const;
		RenderFlowComponent* GetRenderFlowComponent() const;

		// collected cache
		typedef CameraComponentConfig::LightElement LightElement;
		typedef CameraComponentConfig::EnvCubeElement EnvCubeElement;

	protected:
		void DispatchEvent(Event& event, Entity* entity) override;
		void OnTickHost(Engine& engine, Entity* entity);
		void OnTickCameraViewPort(Engine& engine, RenderPort& renderPort, IRender::Queue* queue);
		void UpdateTaskData(Engine& engine, Entity* hostEntity);
		void Instancing(Engine& engine, TaskData& taskData);
		void CommitRenderRequests(Engine& engine, TaskData& taskData, IRender::Queue* queue);

		void CollectLightComponent(Engine& engine, LightComponent* lightComponent, std::vector<std::pair<TShared<RenderPolicy>, LightElement> >& lightElements, const MatrixFloat4x4& worldTransform, const TaskData& taskData) const;
		void CollectEnvCubeComponent(EnvCubeComponent* envCubeComponent, std::vector<std::pair<TShared<RenderPolicy>, EnvCubeElement> >& envCubeElements, const MatrixFloat4x4& worldMatrix) const;
		void CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData);
		void CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* rootEntity);
		void CompleteCollect(Engine& engine, TaskData& taskData);
		void UpdateRootMatrices(const MatrixFloat4x4& cameraWorldMatrix);
		void UpdateJitterMatrices(CameraComponentConfig::WorldGlobalData& worldGlobalData);
		MatrixFloat4x4 ComputeSmoothTrackTransform() const;

	protected:
		// TaskDatas
		TShared<TaskData> prevTaskData;
		TShared<TaskData> nextTaskData;

		// Runtime Resource
		TShared<RenderFlowComponent> renderFlowComponent;
		String cameraViewPortName;
		Entity* rootEntity;
		uint32_t collectedEntityCount;
		uint32_t collectedVisibleEntityCount;
		uint32_t collectedTriangleCount;
		uint32_t jitterIndex;

		// applied if CAMERACOMPONENT_SMOOTH_TRACK enabled
		struct State {
			Quaternion<float> rotation;
			Float3 translation;
			Float3 scale;
		} targetState, currentState;

	public:
		float viewDistance;
	};
}

