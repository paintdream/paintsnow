// SpaceTraversal.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-25
//

#ifndef __SPACE_TRAVERSAL_H__
#define __SPACE_TRAVERSAL_H__

#include "../../../../Core/Template/TProxy.h"
#include "../Renderable/RenderableComponent.h"
#include "../Space/SpaceComponent.h"

namespace PaintsNow {
	namespace NsMythForest {
		template <class T, class Config>
		class SpaceTraversal {
		public:
			typedef typename Config::TaskData TaskData;
			typedef typename Config::WorldInstanceData WorldInstanceData;
			typedef typename Config::CaptureData CaptureData;

			template <class D>
			void CollectComponentsFromEntityTree(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* rootEntity, D ordered) {
				WorldInstanceData nextWorldInstancedData = instanceData;
				const WorldInstanceData& subWorldInstancedData = getboolean<D>::value ? nextWorldInstancedData : instanceData;

				for (Entity* entity = rootEntity; entity != nullptr; entity = entity->Right()) {
					if (!getboolean<D>::value && !captureData(subWorldInstancedData.boundingBox)) break;

					IMemory::PrefetchRead(entity->Left());
					IMemory::PrefetchRead(entity->Right());

					Tiny::FLAG flag = entity->Flag().load(std::memory_order_relaxed);
					if ((flag & Tiny::TINY_ACTIVATED) && captureData(entity->GetKey())) {
						(static_cast<T*>(this))->CollectComponents(engine, taskData, instanceData, captureData, entity);
					}

					// Cull left & right 
					if (getboolean<D>::value) {
						uint32_t index = entity->GetPivotIndex();
						const float* source = &(const_cast<Float3Pair&>(entity->GetKey()).first.x());
						float* target = &nextWorldInstancedData.boundingBox.first.x();
						target[index] = source[index];
					}

					// left part
					if (entity->Left() != nullptr) {
						CollectComponentsFromEntityTree(engine, taskData, subWorldInstancedData, captureData, static_cast<Entity*>(entity->Left()), ordered);
					}
				}
			}

			void CollectComponentsFromSpace(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, SpaceComponent* spaceComponent) {
				if ((spaceComponent->Flag() & Component::COMPONENT_LOCALIZED_WARP) && spaceComponent->GetWarpIndex() != engine.GetKernel().GetCurrentWarpIndex()) {
					spaceComponent->QueueRoutine(engine, CreateTaskContextFree(Wrap(this, &SpaceTraversal<T, Config>::CollectComponentsFromSpace), std::ref(engine), std::ref(taskData), std::ref(instanceData), std::ref(captureData), spaceComponent));
				} else {
					Entity* spaceRoot = spaceComponent->GetRootEntity();
					if (spaceRoot != nullptr) {
						// Make new world instance data
						WorldInstanceData subWorldInstancedData = instanceData;
						// update bounding box
						subWorldInstancedData.boundingBox = spaceComponent->GetBoundingBox();
						if (spaceComponent->Flag() & SpaceComponent::SPACECOMPONENT_ORDERED) {
							CollectComponentsFromEntityTree(engine, taskData, subWorldInstancedData, captureData, spaceRoot, std::true_type());
						} else {
							CollectComponentsFromEntityTree(engine, taskData, subWorldInstancedData, captureData, spaceRoot, std::false_type());
						}
					}

					TAtomic<uint32_t>& pendingCount = reinterpret_cast<TAtomic<uint32_t>&>(taskData.pendingCount);
					if (--pendingCount == 0) {
						(static_cast<T*>(this))->CompleteCollect(engine, taskData);
					}
				}
			}

			void CollectComponentsFromEntity(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity) {
				const std::vector<Component*>& components = entity->GetComponents();
				TAtomic<uint32_t>& pendingCount = reinterpret_cast<TAtomic<uint32_t>&>(taskData.pendingCount);

				++pendingCount;
				for (size_t i = 0; i < components.size(); i++) {
					Component* component = components[i];
					if (component != nullptr && (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE)) {
						SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
						++pendingCount;
						CollectComponentsFromSpace(engine, taskData, instanceData, captureData, spaceComponent);
					}
				}

				if (pendingCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
					(static_cast<T*>(this))->CompleteCollect(engine, taskData);
				}
			}
		};
	}
}

#endif // __SPACE_TRAVERSAL_H__