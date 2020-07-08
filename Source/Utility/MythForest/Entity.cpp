#include "Entity.h"
#include "Component.h"
#include "Engine.h"
#include "../../Core/Interface/IMemory.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

Entity::Entity(Engine& engine) {
	Flag().fetch_or(Tiny::TINY_ACTIVATED, std::memory_order_acquire);
	SetEngineInternal(engine);
	engine.NotifyEntityConstruct(this);

	assert(QueryInterface(UniqueType<WarpTiny>()) != nullptr);
}

Entity::~Entity() {
	assert(components.empty());
	Engine& engine = GetEngineInternal();
	engine.NotifyEntityDestruct(this);
}

static void InvokeClearComponentsAndRelease(void* request, bool run, Engine& engine, Entity* entity) {
	uint32_t orgIndex = engine.GetKernel().GetCurrentWarpIndex();
	uint32_t warpIndex = entity->GetWarpIndex();

	if (!run) {
		if (orgIndex != warpIndex) {
			bool result = engine.GetKernel().taskQueueGrid[warpIndex].PreemptExecution();
			assert(result);
		}
	} else {
		assert(engine.GetKernel().GetCurrentWarpIndex() == entity->GetWarpIndex());
	}

	entity->ClearComponents(engine);
	entity->ReleaseObject();

	if (!run) {
		if (orgIndex != ~(uint32_t)0 && orgIndex != warpIndex) {
			engine.GetKernel().taskQueueGrid[orgIndex].PreemptExecution();
		}
	}
}

void Entity::ReleaseObject() {
	if (GetExtReferCount() == 0) {
		if (Flag() & ENTITY_STORE_ENGINE) {
			if (!components.empty()) {
				Engine& engine = GetEngineInternal();
				Kernel& kernel = engine.GetKernel();
				if (kernel.GetCurrentWarpIndex() == GetWarpIndex()) {
					ClearComponents(engine);
				} else {
					kernel.QueueRoutine(this, CreateTask(Wrap(InvokeClearComponentsAndRelease), std::ref(engine), this));
					return;
				}
			}
		} else {
			assert(components.empty());
		}
	}

	Unit::ReleaseObject();
}

void Entity::SetEngineInternal(Engine& engine) {
	// reuse parent node as engine pointer
	assert(!(Flag() & ENTITY_STORE_ENGINE));
	Flag().fetch_or(ENTITY_STORE_ENGINE, std::memory_order_acquire);

	reinterpret_cast<Engine*&>(*(void**)&_parentNode) = &engine;
}

void Entity::CleanupEngineInternal() {
	Flag().fetch_and(~ENTITY_STORE_ENGINE, std::memory_order_release);

	reinterpret_cast<Engine*&>(*(void**)&_parentNode) = nullptr;
}

Engine& Entity::GetEngineInternal() const {
	assert(GetParent() != nullptr);
	assert(Flag() & ENTITY_STORE_ENGINE);

	return *reinterpret_cast<Engine*>(*(void**)&_parentNode);
}

void Entity::InitializeComponent(Engine& engine, Component* component) {
	component->ReferenceObject();
	component->Initialize(engine, this);

	// add component mask
	Flag().fetch_or((component->GetEntityFlagMask() | TINY_MODIFIED), std::memory_order_acquire);

	if (Flag() & ENTITY_HAS_TACH_EVENTS) {
		Event event(engine, Event::EVENT_ATTACH_COMPONENT, this, component);
		PostEvent(event);
	}
}

void Entity::UninitializeComponent(Engine& engine, Component* component) {
	if (Flag() & ENTITY_HAS_TACH_EVENTS) {
		Event event(engine, Event::EVENT_DETACH_COMPONENT, this, component);
		PostEvent(event);
	}

	component->Uninitialize(engine, this);
	component->ReleaseObject();
}

void Entity::AddComponent(Engine& engine, Component* component) {
	assert((component->Flag() & Component::COMPONENT_LOCALIZED_WARP) || component->GetWarpIndex() == GetWarpIndex());
	uint32_t id = component->GetQuickUniqueID();
	if (id != ~(uint32_t)0) {
		assert(component->Flag() & TINY_UNIQUE);
		if (id >= components.size()) {
			components.resize(id + 1, nullptr);
			components[id] = component;
			InitializeComponent(engine, component);
		} else {
			Component* target = components[id];
			if (target == component) return;

			if (target != nullptr) {
				if (target->GetUnique() != component->GetUnique()) {
					components.emplace_back(target);
				} else {
					UninitializeComponent(engine, target);
				}
			}

			components[id] = component;
			InitializeComponent(engine, component);
		}
	} else if (component->Flag() & Tiny::TINY_UNIQUE) {
		Unique unique = component->GetUnique();
		for (size_t i = 0; i < components.size(); i++) {
			Component*& c = components[i];
			if (c == nullptr) continue;
			if (c == component) return; // self

			if (c->GetUnique() == unique) {
				UninitializeComponent(engine, c);
				c = component;
				InitializeComponent(engine, component);
				return;
			}
		}

		components.emplace_back(component);
		InitializeComponent(engine, component);
	} else {
		if (Flag() & ENTITY_STORE_NULLSLOT) {
			for (size_t i = 0; i < components.size(); i++) {
				Component*& target = components[i];
				if (target == nullptr) {
					target = component;
					InitializeComponent(engine, component);
					return;
				}
			}

			Flag().fetch_and(~ENTITY_STORE_NULLSLOT, std::memory_order_release);
		}

		components.emplace_back(component);
		InitializeComponent(engine, component);
	}
}

void Entity::RemoveComponent(Engine& engine, Component* component) {
	assert(component != nullptr);
	uint32_t id = component->GetQuickUniqueID();
	if (id != ~(uint32_t)0) {
		assert(component->Flag() & TINY_UNIQUE);
		if (id < components.size()) {
			Component*& target = components[id];
			if (target == component) {
				UninitializeComponent(engine, target);
				target = nullptr;

				Flag().fetch_or(ENTITY_STORE_NULLSLOT, std::memory_order_acquire);
			}
		}
	} else {
		for (size_t i = 0; i < components.size(); i++) {
			Component*& target = components[i];
			if (target == component) {
				UninitializeComponent(engine, target);

				if (i == components.size() - 1) {
					while (i != 0 && components[i - 1] == nullptr) --i;
					components.erase(components.begin() + i, components.end());
				} else {
					target = nullptr;
					Flag().fetch_or(ENTITY_STORE_NULLSLOT, std::memory_order_acquire);
				}

				break;
			}
		}
	}
}

void Entity::UpdateEntityFlags() {
	FLAG p = 0, m;
	FLAG f = 0;
	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			f |= components[i]->GetEntityFlagMask();
		}
	}

	do {
		p = Flag().load(std::memory_order_acquire);
		m = (p & ~ENTITY_HAS_ALL) | f;
	} while (!flag.compare_exchange_weak(p, m, std::memory_order_relaxed));
}

const std::vector<Component*>& Entity::GetComponents() const {
	return components;
}

Component* Entity::GetUniqueComponent(Unique unique) const {
	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr && component->GetUnique()->IsClass(unique)) {
			assert(component->Flag() & Tiny::TINY_UNIQUE);
			return component;
		}
	}

	return nullptr;
}

void Entity::ClearComponents(Engine& engine) {
	if (Flag() & ENTITY_HAS_TACH_EVENTS) {
		for (size_t i = 0; i < components.size(); i++) {
			Component* component = components[i];
			if (component != nullptr) {
				Event event(engine, Event::EVENT_DETACH_COMPONENT, this, component);
				PostEvent(event);
			}
		}
	}

	std::vector<Component*> temp;
	std::swap(temp, components);

	for (size_t i = 0; i < temp.size(); i++) {
		Component* component = temp[i];
		if (component != nullptr) {
			UninitializeComponent(engine, component);
		}
	}
}

void Entity::PostEvent(Event& event) {
	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			component->DispatchEvent(event, this);
		}
	}
}

TObject<IReflect>& Entity::operator () (IReflect& reflect) {
	// BaseClass::operator () (reflect);
	ReflectClass(Entity)[ReflectInterface(Unit)];

	if (reflect.IsReflectProperty()) {
		ReflectProperty(components);
	}

	return *this;
}

bool Entity::IsOrphan() const {
	return !!(Flag() & ENTITY_STORE_ENGINE);
}

uint32_t Entity::GetPivotIndex() const {
	return GetIndex();
}

void Entity::UpdateBoundingBox(Engine& engine) {
	if (Flag() & TINY_MODIFIED) {
		assert(IsOrphan());

		if (IsOrphan()) {
			Float3Pair box(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
			// Since Transform component is always the first component (if exists)
			// We iterate components from end to begin
			for (size_t i = components.size(); i > 0; i--) {
				Component* component = components[i - 1];
				if (component != nullptr) {
					component->UpdateBoundingBox(engine, box);
				}
			}

			if (box.second.x() - box.first.x() >= 0) {
				SetKey(box);
			}
		}

		Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	}
}