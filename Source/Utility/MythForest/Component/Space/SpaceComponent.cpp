#include "SpaceComponent.h"
#include "../../Entity.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

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
	if (Flag().load(std::memory_order_relaxed) & COMPONENT_OVERRIDE_WARP) {
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
	OPTICK_EVENT();
	assert(Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED);
	if (!(Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED)) {
		return false;
	}

	// already managed by other space, must detach it before calling me.
	if (!entity->IsOrphan()) {
		return false;
	}

	if (!ValidateCycleReferences(entity)) {
		assert(false);
		return false; // cycle reference detected!
	}

	assert(GetWarpIndex() == entity->GetWarpIndex());

#ifdef _DEBUG
	assert(hostEntity != nullptr);
	engine.NotifyUnitAttach(entity, hostEntity);
#endif

	// update bounding box
	entity->UpdateBoundingBox(engine, false);
	entity->ReferenceObject();
	// cleanup has_engine flag
	entity->CleanupEngineInternal();
	entity->SetIndex(entityCount % 6);

	if (rootEntity != nullptr) {
		if (Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_ORDERED) {
			rootEntity->Attach(entity);
		} else {
			// chain
			entity->Right() = rootEntity;
			assert(rootEntity->GetParent() == nullptr);
			rootEntity->SetParent(entity);
			rootEntity = entity;
		}

		Math::Union(boundingBox, entity->GetKey().first);
		Math::Union(boundingBox, entity->GetKey().second);
	} else {
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
	OPTICK_EVENT();
	DispatchEventRecursive(const_cast<Event&>(event), GetRootEntity());
}

void SpaceComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
	// forward tick only
	if ((Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_FORWARD_EVENT_TICK) && event.eventID == Event::EVENT_TICK) {
		if (Flag().load(std::memory_order_relaxed) & COMPONENT_OVERRIDE_WARP) {
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

bool SpaceComponent::RecursiveValidateCycleReferences(Entity* entity) {
	if (entity == nullptr) {
		return true;
	}

	for (Entity* p = entity; p != nullptr; p = p->Right()) {
		if (!ValidateCycleReferences(p)) {
			return false;
		}

		if (!RecursiveValidateCycleReferences(p->Left())) {
			return false;
		}
	}

	return true;
}

bool SpaceComponent::ValidateCycleReferences(Entity* entity) {
#ifdef _DEBUG
	// only checks in debug version.
	const std::vector<Component*>& components = entity->GetComponents();
	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			if (component == this) {
				return false; // cycle detected!!
			}

			SpaceComponent* subSpaceComponent = component->QueryInterface(UniqueType<SpaceComponent>());
			if (subSpaceComponent != nullptr) {
				if (!RecursiveValidateCycleReferences(subSpaceComponent->GetRootEntity())) {
					return false;
				}
			}
		}
	}
#endif

	return true;
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

bool SpaceComponent::Remove(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	if (!ValidateEntity(entity)) {
		return false;
	}

	Entity* newRoot = nullptr;
	if (Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_ORDERED) {
		SelectRemove selectRemove;
		rootEntity = static_cast<Entity*>(entity->Detach(selectRemove));
	} else if (rootEntity == entity) {
		rootEntity = entity->Right();
	} else {
		assert(entity->Parent() != nullptr);
		entity->Parent()->Right() = entity->Right();
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
	CollectEntityHandler(std::vector<TShared<Entity> >& t, const Float3Pair& b) : entityList(t), box(b) {}
	const Float3Pair& box;

	bool operator () (Entity::Type& tree) {
		if (Math::Overlap(box, tree.GetKey())) {
			entityList.emplace_back(static_cast<Entity*>(&tree));
		}

		return true;
	}

	std::vector<TShared<Entity> >& entityList;
};

void SpaceComponent::QueryEntities(std::vector<TShared<Entity> >& entityList, const Float3Pair& box) {
	CollectEntityHandler handler(entityList, box);
	if (rootEntity != nullptr) {
		rootEntity->Query(std::true_type(), Float3Pair(box), handler);
	}
}

void SpaceComponent::RoutineUpdateBoundingBoxRecursive(Engine& engine, Float3Pair& box, Entity* entity, bool subEntity) {
	while (entity != nullptr) {
		if (subEntity) {
			entity->UpdateBoundingBox(engine, subEntity);
		}

		const Float3Pair& eb = entity->GetKey();

		assert(eb.first.x() > -FLT_MAX && eb.second.x() < FLT_MAX);
		assert(eb.first.y() > -FLT_MAX && eb.second.y() < FLT_MAX);
		assert(eb.first.z() > -FLT_MAX && eb.second.z() < FLT_MAX);
		Math::Union(box, eb.first);
		Math::Union(box, eb.second);
		assert(box.first.x() > -FLT_MAX && box.second.x() < FLT_MAX);
		assert(box.first.y() > -FLT_MAX && box.second.y() < FLT_MAX);
		assert(box.first.z() > -FLT_MAX && box.second.z() < FLT_MAX);

		RoutineUpdateBoundingBoxRecursive(engine, box, entity->Left(), subEntity);
		assert(box.first.x() > -FLT_MAX && box.second.x() < FLT_MAX);
		assert(box.first.y() > -FLT_MAX && box.second.y() < FLT_MAX);
		assert(box.first.z() > -FLT_MAX && box.second.z() < FLT_MAX);
		entity = entity->Right();
	}
}

void SpaceComponent::RoutineUpdateBoundingBox(Engine& engine, Float3Pair& box, bool subEntity) {
	RoutineUpdateBoundingBoxRecursive(engine, box, rootEntity, subEntity);
	assert(box.first.x() > -FLT_MAX && box.second.x() < FLT_MAX);
	assert(box.first.y() > -FLT_MAX && box.second.y() < FLT_MAX);
	assert(box.first.z() > -FLT_MAX && box.second.z() < FLT_MAX);
	boundingBox = box;
	Flag().fetch_and(~Tiny::TINY_UPDATING, std::memory_order_release);
}

template <class D>
void RaycastInternal(Entity* root, Component::RaycastTask& task, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio, const Float3Pair& b, D d) {
	Float3Pair bound = b;
	for (Entity* entity = root; entity != nullptr; entity = entity->Right()) {
		if (!getboolean<D>::value) {
			TVector<float, 2> intersect = Math::IntersectBox(bound, ray);
			if (intersect[1] < 0.0f || intersect[0] > intersect[1])
				break;

			// evaluate possible distance
			if (task.clipOffDistance != FLT_MAX) {
				float nearest = Math::Length(bound.second - ray.first) - Math::Length(bound.second - bound.first) * 0.5f;
				if (nearest >= task.clipOffDistance)
					break;
			}
		}

		IMemory::PrefetchRead(entity->Left());
		IMemory::PrefetchRead(entity->Right());

		Component::RaycastForEntity(task, ray, transform, entity);

		if (getboolean<D>::value) {
			uint32_t index = entity->GetIndex();
			float save = Entity::Meta::SplitPush(std::true_type(), bound, entity->GetKey(), index);
			if (entity->Left() != nullptr) {
				RaycastInternal(entity->Left(), task, ray, transform, parent, ratio, bound, d);
			}

			Entity::Meta::SplitPop(bound, index, save);
		} else if (entity->Left() != nullptr) {
			RaycastInternal(entity->Left(), task, ray, transform, parent, ratio, bound, d);
		}
	}
}

float SpaceComponent::RoutineRaycast(RaycastTaskWarp& task, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio) const {
	ratio = Raycast(task, ray, transform, parent, ratio);
	if (parent != nullptr) {
		parent->ReleaseObject();
	}

	task.RemovePendingTask();
	return ratio;
}

float SpaceComponent::Raycast(RaycastTask& rayCastTask, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio) const {
	OPTICK_EVENT();

	if (rayCastTask.Flag().load(std::memory_order_relaxed) & Component::RaycastTask::RAYCASTTASK_IGNORE_WARP) {
		if (Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_ORDERED) {
			RaycastInternal(rootEntity, rayCastTask, ray, transform, parent, ratio, boundingBox, std::true_type());
		} else {
			RaycastInternal(rootEntity, rayCastTask, ray, transform, parent, ratio, boundingBox, std::false_type());
		}

		return ratio;
	} else {
		RaycastTaskWarp& task = static_cast<RaycastTaskWarp&>(rayCastTask);
		if (!(Flag().load(std::memory_order_relaxed) & COMPONENT_OVERRIDE_WARP) || task.GetEngine().GetKernel().GetCurrentWarpIndex() == GetWarpIndex()) {
			if (Flag().load(std::memory_order_relaxed) & SPACECOMPONENT_ORDERED) {
				RaycastInternal(rootEntity, task, ray, transform, parent, ratio, boundingBox, std::true_type());
			} else {
				RaycastInternal(rootEntity, task, ray, transform, parent, ratio, boundingBox, std::false_type());
			}
		} else {
			if (parent != nullptr) {
				parent->ReferenceObject();
			}

			task.AddPendingTask();
			task.GetEngine().GetKernel().QueueRoutine(const_cast<SpaceComponent*>(this), CreateTaskContextFree(Wrap(this, &SpaceComponent::RoutineRaycast), std::ref(task), ray, transform, parent, ratio));
		}

		return ratio;
	}
}

void SpaceComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) {
	if (rootEntity == nullptr) return;
	OPTICK_EVENT();

	Tiny::FLAG flag = Flag().load(std::memory_order_relaxed);

	if (!(flag & TINY_PINNED)) {
		Float3Pair newBox(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
		Kernel& kernel = engine.bridgeSunset.GetKernel();
		uint32_t warp = kernel.GetCurrentWarpIndex();

		if ((flag & COMPONENT_OVERRIDE_WARP) && warp != GetWarpIndex()) {
			if (!(Flag().fetch_or(TINY_UPDATING, std::memory_order_acquire) & TINY_UPDATING)) {
				QueueRoutine(engine, CreateTaskContextFree(Wrap(this, &SpaceComponent::RoutineUpdateBoundingBox), std::ref(engine), std::ref(newBox), recursive));

				// block until update finishes
				kernel.YieldCurrentWarp();
				kernel.GetThreadPool().PollWait(Flag(), TINY_UPDATING, 0, 5);
				kernel.WaitWarp(warp);
			}
		} else {
			RoutineUpdateBoundingBox(engine, newBox, recursive);
		}

		if (newBox.first.x() <= newBox.second.x()) {
			Math::Union(box, newBox.first);
			Math::Union(box, newBox.second);
		}
	} else {
		if (boundingBox.first.x() <= boundingBox.second.x()) {
			Math::Union(box, boundingBox.first);
			Math::Union(box, boundingBox.second);
		}
	}
}

const Float3Pair& SpaceComponent::GetBoundingBox() const {
	return boundingBox;
}
