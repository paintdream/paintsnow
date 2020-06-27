// SpaceComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-10
//

#ifndef __SPACECOMPONENT_H__
#define __SPACECOMPONENT_H__

#include "../../Component.h"
#include "../../Entity.h"

namespace PaintsNow {
	namespace NsMythForest {
		class SpaceComponent : public TAllocatedTiny<SpaceComponent, Component> {
		public:
			SpaceComponent(bool sorted = true);
			virtual ~SpaceComponent();

			enum {
				SPACECOMPONENT_ORDERED = COMPONENT_CUSTOM_BEGIN,
				SPACECOMPONENT_FORWARD_EVENT_TICK = COMPONENT_CUSTOM_BEGIN << 1,
				SPACECOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 2
			};

			void QueueRoutine(Engine& engine, ITask* task);
			void QueryEntities(std::vector<TShared<Entity> >& entities, const Float3Pair& box);
			bool Insert(Engine& engine, Entity* entity);
			bool Remove(Engine& engine, Entity* entity);
			void RemoveAll(Engine& engine);
			uint32_t GetEntityCount() const;

			Entity* GetRootEntity() const;
			void SetRootEntity(Entity* entity);
			virtual FLAG GetEntityFlagMask() const override;
			virtual void DispatchEvent(Event& event, Entity* entity) override;
			virtual void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
			const Float3Pair& GetBoundingBox() const;
			virtual float Raycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const override;
			float RoutineRaycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const;

		protected:
			virtual void Initialize(Engine& engine, Entity* entity) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
			void AttachUnsorted(Entity* parent, Entity* child, uint32_t seed);
			Entity* DetachUnsorted(Entity* child);
			void UpdateEntityWarpIndex(Entity* entity);
			void RoutineUpdateEntityWarpIndex(Engine& engine, Entity* entity, bool insert);
			void RoutineInsertEntity(Engine& engine, Entity* entity);
			void RoutineUpdateBoundingBox();
			void RoutineUpdateBoundingBoxRecursive(Float3Pair& box, Entity* entity);
			void RoutineDispatchEvent(const Event& event);
			void FastRemoveNode(Engine& engine, Entity* entity);
			bool ValidateEntity(Entity* entity);

		protected:
			Float3Pair boundingBox;
			Entity* rootEntity;
			uint32_t entityCount;
		};
	}
}

#endif // __SPACECOMPONENT_H__
