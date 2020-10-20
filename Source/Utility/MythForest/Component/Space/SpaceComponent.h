// SpaceComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-10
//

#pragma once
#include "../../Component.h"
#include "../../Entity.h"

namespace PaintsNow {
	class SpaceComponent : public TAllocatedTiny<SpaceComponent, Component> {
	public:
		SpaceComponent(bool sorted = true);
		~SpaceComponent() override;

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
		FLAG GetEntityFlagMask() const override;
		void DispatchEvent(Event& event, Entity* entity) override;
		void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
		const Float3Pair& GetBoundingBox() const;
		float Raycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const override;

	protected:
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		void AttachUnsorted(Entity* parent, Entity* child, uint32_t seed);
		Entity* DetachUnsorted(Entity* child);
		float RoutineRaycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const;
		void RoutineUpdateBoundingBox();
		void RoutineUpdateBoundingBoxRecursive(Float3Pair& box, Entity* entity);
		void RoutineDispatchEvent(const Event& event);
		void FastRemoveNode(Engine& engine, Entity* entity);
		bool ValidateEntity(Entity* entity);

	protected:
		Float3Pair boundingBox;
		Entity* rootEntity;
		uint32_t entityCount;
#ifdef _DEBUG
		Entity* hostEntity;
#endif
	};
}

