// LightComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __LIGHTCOMPONENT_H__
#define __LIGHTCOMPONENT_H__

#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../Animation/AnimationComponent.h"
#include "../Stream/StreamComponent.h"
#include "../Explorer/SpaceTraversal.h"
#include "../Explorer/CameraCuller.h"
#include "../../../SnowyStream/Resource/TextureResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		struct LightComponentConfig {
			struct CaptureData : public FrustrumCuller {
				Bytes visData;
			};

			struct WorldInstanceData : public TReflected<WorldInstanceData, ZPassBase::PartialData> {
				WorldInstanceData() {}
				virtual TObject<IReflect>& operator () (IReflect& reflect);

				MatrixFloat4x4 worldMatrix;
				Float3Pair boundingBox;
				TShared<AnimationComponent> animationComponent;
			};

			struct InstanceKey {
				inline bool operator == (const InstanceKey& rhs) const {
					return memcmp(this, &rhs, sizeof(*this)) == 0;
				}

				size_t renderKey;
				size_t animationKey;
			};

			struct HashInstanceKey {
				inline size_t operator () (const InstanceKey& key) const {
					return key.renderKey ^ key.animationKey;
				}
			};

			struct InstanceGroup {
				InstanceGroup() : instanceCount(0), instanceUpdater(nullptr) {}
				ZPassBase::PartialUpdater* instanceUpdater;
				std::vector<Bytes> instancedData;
				IRender::Resource::DrawCallDescription drawCallDescription;
				TShared<AnimationComponent> animationComponent;
				uint32_t instanceCount;
			};

			struct TaskData : public TReflected<TaskData, SharedTiny> {
				TaskData(uint32_t warpCount);
				virtual ~TaskData();
				void Cleanup(IRender& render);
				void Destroy(IRender& render);

				virtual TObject<IReflect>& operator () (IReflect& reflect) override;

				struct WarpData {
					typedef unordered_map<InstanceKey, InstanceGroup, HashInstanceKey> InstanceGroupMap;
					WarpData();
					InstanceGroupMap instanceGroups;
					IRender::Queue* renderQueue;

					struct GlobalBufferItem {
						ZPassBase::PartialUpdater instanceUpdater;
						std::vector<IRender::Resource*> buffers;
					};

					std::map<NsSnowyStream::ShaderResource*, GlobalBufferItem> worldGlobalBufferMap;
					std::vector<NsSnowyStream::IDrawCallProvider::DataUpdater*> dataUpdaters;
				};

				std::vector<WarpData> warpData;
				TAtomic<uint32_t> pendingCount;
				OrthoCamera camera;
			};
		};

		class LightComponent : public TAllocatedTiny<LightComponent, RenderableComponent>, SpaceTraversal<LightComponent, LightComponentConfig> {
		public:
			LightComponent();
			friend class SpaceTraversal<LightComponent, LightComponentConfig>;

			typedef typename LightComponentConfig::InstanceKey InstanceKey;
			typedef typename LightComponentConfig::HashInstanceKey HashInstanceKey;
			typedef typename LightComponentConfig::InstanceGroup InstanceGroup;

			enum {
				LIGHTCOMPONENT_DIRECTIONAL = COMPONENT_CUSTOM_BEGIN,
				LIGHTCOMPONENT_CAST_SHADOW = COMPONENT_CUSTOM_BEGIN << 1,
				LIGHTCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 2
			};

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual FLAG GetEntityFlagMask() const override;
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData);
			virtual void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
			void BindShadowStream(uint32_t layer, TShared<StreamComponent> streamComponent, const Float2& gridSize);
			const Float3& GetColor() const;
			void SetColor(const Float3& color);
			float GetAttenuation() const;
			void SetAttenuation(float value);
			const Float3& GetRange() const;
			void SetRange(const Float3& range);

			// float spotAngle;
			// float temperature;
			struct ShadowLayer {
				TShared<StreamComponent> streamComponent;
				TShared<TaskData> taskData;
				Float2 gridSize;
			};

			class ShadowGrid : public TReflected<ShadowGrid, SharedTiny> {
			public:
				TShared<NsSnowyStream::TextureResource> texture;
				uint32_t layer;
			};

		protected:
			void ReplaceStreamComponent(ShadowLayer& shadowLayer, TShared<StreamComponent> streamComponent);
			TShared<SharedTiny> StreamLoadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny);
			TShared<SharedTiny> StreamUnloadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny);

			void CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData);
			void CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* rootEntity);
			void CompleteCollect(Engine& engine, TaskData& taskData);

			Float3 color;
			float attenuation;
			Float3 range;
			std::vector<ShadowLayer> shadowLayers;
		};
	}
}


#endif // __LIGHTCOMPONENT_H__
