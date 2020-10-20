#include "SpaceComponent.h"
#include "../../Entity.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../../../Core/Interface/IMemory.h"

using namespace PaintsNow;

SpaceComponent::SpaceComponent(bool sorted) : rootEntity(nullptr), entityCount(0), boundingBox(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX)) {
	if (sorted) {
		Flag().fetch_or(SPACECOMPONENT_ORDERED, std::memory_order_relaxed);
	}

	Flag().fetch_or(COMPONENT_SHARED, std::memory_order_relaxed);

#ifdef _DEBUG
	hostEntity = nullptr;
#endif
}

SpaceComponent::~SpaceComponent() {}

void SpaceComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);

#ifdef _DEBUG
	assert(hostEntity == nullptr);
	hostEntity = entity;
#endif
}

static void InvokeRemoveAll(void* context, bool run, Engine& engine, SpaceComponent* spaceComponent) {
	spaceComponent->RemoveAll(engine);
}

void SpaceComponent::Uninitialize(Engine& engine, Entity* entity) {
	if (Flag().load(std::memory_order_relaxed) & COMPONENT_LOCALIZED_WARP) {
		QueueRoutine(engine, CreateTask(Wrap(InvokeRemoveAll), std::ref(engine), this));
	} else {
		RemoveAll(engine);
	}

	BaseClass::Uninitialize(engine, entity);

#ifdef _DEBUG
	assert(hostEntity == entity);
	hostEntity = nullptr;
#endif
}

void SpaceComponent::QueueRoutine(Engine& engine, ITask* task) {
	engine.GetKernel().QueueRoutine(this, task);
}

bool SpaceComponent::Insert(Engine& engine, Entity* entity) {
	assert(Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED);
	if (!(Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED)) {
		return false;
	}

	// already managed by other space, must detach it before calling me.
	if (!entity->IsOrphan()) {
		return false;
	}

	assert(GetWarpIndex() == entity->GetWarpIndex());

#ifdef _DEBUG
	assert(hostEntity != nullptr);
	engine.NotifyUnitAttach(entity, hostEntity);
#endif

	// update bounding box
	entity->UpdateBoundingBox(engine);
	entity->ReferenceObject();
	// cleanup has_engine flag
	entity->CleanupEngineInternal();
	entity->SetIndex(entityCount % 6);

	if (rootEntity)
	{
		if (Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_ORDERED)
		{
			rootEntity->Attach(entity);
		}
		else
		{
			AttachUnsorted(rootEntity, entity, entityCount);
		}

		Union(boundingBox, entity->GetKey().first);
		Union(boundingBox, entity->GetKey().second);
	}
	else
	{
		rootEntity = entity;
		boundingBox = entity->GetKey();
	}

	++entityCount;

	return true;
}

Tiny::FLAG SpaceComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_SPACE;
}

inline void DispatchEventRecursive(Event& event, Entity* entity) {
	while (entity != nullptr) {
		IMemory::PrefetchRead(entity->Left());
		IMemory::PrefetchRead(entity->Right());
		entity->PostEvent(event, ~(Tiny::FLAG)0);
		DispatchEventRecursive(event, entity->Left());
		entity = entity->Right();
	}
}

void SpaceComponent::RoutineDispatchEvent(const Event& event) {
	DispatchEventRecursive(const_cast<Event&>(event), GetRootEntity());
}

void SpaceComponent::DispatchEvent(Event& event, Entity* entity) {
	// forward tick only
	if ((Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_FORWARD_EVENT_TICK) && event.eventID == Event::EVENT_TICK) {
		if (Flag().load(std::memory_order_relaxed) & COMPONENT_LOCALIZED_WARP) {
			QueueRoutine(event.engine, CreateTaskContextFree(Wrap(this, &SpaceComponent::RoutineDispatchEvent), event));
		} else {
			RoutineDispatchEvent(event);
		}
	}
}

Entity* SpaceComponent::GetRootEntity() const {
	return rootEntity;
}

void SpaceComponent::SetRootEntity(Entity* root) {
	rootEntity = root;
}

bool SpaceComponent::ValidateEntity(Entity* entity) {
	// Examine parent
	Entity* parent = entity;
	while (parent != nullptr && parent != rootEntity) {
		parent = static_cast<Entity*>(parent->GetParent());
	}

	return parent == rootEntity;
}

struct SelectRemove {
	inline bool operator () (Entity::Type* left, Entity::Type* right) const {
		assert(left != nullptr && right != nullptr);
		// TODO: use better policy
		return left < right;
	}
};

void SpaceComponent::AttachUnsorted(Entity* parent, Entity* entity, uint32_t seed) {
	if (parent->Left() == nullptr) {
		parent->Left() = entity;
		entity->SetParent(parent);
	} else if (parent->Right() == nullptr) {
		parent->Right() = entity;
		entity->SetParent(parent);
	} else {
		AttachUnsorted(seed & 1 ? parent->Left() : parent->Right(), entity, seed >> 1);
	}
}

Entity* SpaceComponent::DetachUnsorted(Entity* entity) {
	Entity* parent = entity->Parent();
	Entity* left = entity->Left();
	Entity* right = entity->Right();

	if (left != nullptr) {
		if (right != nullptr) {
			left = entityCount & 1 ? right : left;
		}
	} else {
		left = right;
	}

	if (parent != nullptr) {
		if (entity == parent->Left()) {
			parent->Left() = left;
		} else {
			parent->Right() = left;
		}

		return nullptr;
	} else {
		return left;
	}
}

bool SpaceComponent::Remove(Engine& engine, Entity* entity) {
	if (!ValidateEntity(entity)) {
		return false;
	}

	Entity* newRoot = nullptr;
	if (Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_ORDERED) {
		SelectRemove selectRemove;
		newRoot = static_cast<Entity*>(entity->Detach(selectRemove));
	} else {
		newRoot = DetachUnsorted(entity);
	}

	if (newRoot != nullptr || entity == rootEntity) {
		rootEntity = newRoot;
	}

	if (--entityCount == 0) {
		boundingBox = Float3Pair(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
	}

#ifdef _DEBUG
	engine.NotifyUnitDetach(entity);
#endif

	entity->SetEngineInternal(engine);
	entity->ReleaseObject();

	return true;
}

uint32_t SpaceComponent::GetEntityCount() const {
	return entityCount;
}

void SpaceComponent::RemoveAll(Engine& engine) {
	// Remove all references ...
	entityCount = 0;
	FastRemoveNode(engine, rootEntity);
	boundingBox = Float3Pair(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
	rootEntity = nullptr;
}

void SpaceComponent::FastRemoveNode(Engine& engine, Entity* entity) {
	while (entity != nullptr) {
		FastRemoveNode(engine, entity->Left());
		Entity* right = entity->Right();
		engine.NotifyUnitDetach(entity);
		entity->SetEngineInternal(engine); // reset engine
		entity->ReleaseObject();
		entity = right;
	}
}

struct CollectEntityHandler {
public:
	CollectEntityHandler(std::vector<TShared<Entity> >& t) : entityList(t) {}

	bool operator () (const Float3Pair& box, Entity::Type& tree) {
		if (Overlap(box, tree.GetKey())) {
			entityList.emplace_back(static_cast<Entity*>(&tree));
		}

		return true;
	}

	std::vector<TShared<Entity> >& entityList;
};

void SpaceComponent::QueryEntities(std::vector<TShared<Entity> >& entityList, const Float3Pair& box) {
	CollectEntityHandler handler(entityList);
	if (rootEntity != nullptr) {
		rootEntity->Query(Float3Pair(box), handler);
	}
}

void SpaceComponent::RoutineUpdateBoundingBoxRecursive(Float3Pair& box, Entity* entity) {
	while (entity != nullptr) {
		// To avoid updating storm.
		// Do not trigger entities' UpdateBoundingBox
		// It should be updated manually by entity's owner
		const Float3Pair& eb = entity->GetKey();
		Union(box, eb.first);
		Union(box, eb.second);

		RoutineUpdateBoundingBoxRecursive(box, entity->Left());
		entity = entity->Right();
	}
}

void SpaceComponent::RoutineUpdateBoundingBox() {
	Float3Pair box(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
	RoutineUpdateBoundingBoxRecursive(box, rootEntity);
	boundingBox = box;
	Flag().fetch_and(~Tiny::TINY_UPDATING, std::memory_order_release);
}

template <class D>
void RaycastInternal(Entity* root, Component::RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio, const Float3Pair& b, D d) {
	Float3Pair bound = b;
	for (Entity* entity = root; entity != nullptr; entity = entity->Right()) {
		if (!getboolean<D>::value && !Math::IntersectBox(bound, ray))
			break;

		IMemory::PrefetchRead(entity->Left());
		IMemory::PrefetchRead(entity->Right());

		Component::RaycastForEntity(task, ray, entity);

		if (getboolean<D>::value) {
			uint32_t index = entity->GetPivotIndex();
			const float* source = &(const_cast<Float3Pair&>(entity->GetKey()).first.x());
			float* target = &bound.first.x();
			if (index < 3) {
				// cull right
				if (entity->Left() != nullptr) {
					RaycastInternal(entity->Left(), task, ray, parent, ratio, bound, d);
				}

				target[index] = source[index]; // pass through
			} else if (entity->Left() != nullptr) {
				// cull left
				float value = target[index];
				target[index] = source[index];
				RaycastInternal(entity->Left(), task, ray, parent, ratio, bound, d);
				target[index] = value;
			}
		} else if (entity->Left() != nullptr) {
			RaycastInternal(entity->Left(), task, ray, parent, ratio, bound, d);
		}
	}
}

float SpaceComponent::RoutineRaycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const {
	ratio = Raycast(task, ray, parent, ratio);
	if (parent != nullptr) {
		parent->ReleaseObject();
	}

	task.RemovePendingTask();
	return ratio;
}

float SpaceComponent::Raycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const {
	if (!(Flag().load(std::memory_order_relaxed) & COMPONENT_LOCALIZED_WARP) || task.GetEngine().GetKernel().GetCurrentWarpIndex() == GetWarpIndex()) {
		if (Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_ORDERED) {
			RaycastInternal(rootEntity, task, ray, parent, ratio, boundingBox, std::true_type());
		} else {
			RaycastInternal(rootEntity, task, ray, parent, ratio, boundingBox, std::false_type());
		}
	} else {
		if (parent != nullptr) {
			parent->ReferenceObject();
		}

		task.AddPendingTask();
		task.GetEngine().GetKernel().QueueRoutine(const_cast<SpaceComponent*>(this), CreateTaskContextFree(Wrap(this, &SpaceComponent::RoutineRaycast), std::ref(task), ray, parent, ratio));
	}

	return ratio;
}

void SpaceComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box) {
	Tiny::FLAG flag = Flag().load(std::memory_order_relaxed);

	if (flag & TINY_MODIFIED) {
		if (flag & COMPONENT_LOCALIZED_WARP) {
			if (!(flag & TINY_UPDATING)) {
				Flag().fetch_or(TINY_UPDATING, std::memory_order_acquire);
				QueueRoutine(engine, CreateTaskContextFree(Wrap(this, &SpaceComponent::RoutineUpdateBoundingBox)));
			}
		} else {
			RoutineUpdateBoundingBox();
		}

		Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	}

	Union(box, boundingBox.first);
	Union(box, boundingBox.second);
}

const Float3Pair& SpaceComponent::GetBoundingBox() const {
	return boundingBox;
}
