#include "ExplorerComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ExplorerComponent::ExplorerComponent() {
#ifdef _DEBUG
	hostEntity = nullptr;
#endif
}

ExplorerComponent::~ExplorerComponent() {}

void ExplorerComponent::Initialize(Engine& engine, Entity* entity) {
	assert(!(Flag() & TINY_ACTIVATED));
	assert(hostEntity == nullptr);
#ifdef _DEBUG
	hostEntity = entity;
#endif
	Component::Initialize(engine, entity);
}

void ExplorerComponent::Uninitialize(Engine& engine, Entity* entity) {
	assert((Flag() & TINY_ACTIVATED));
	Component::Uninitialize(engine, entity);
#ifdef _DEBUG
	hostEntity = nullptr;
#endif
}

static inline bool CheckBit(const uint8_t* p, size_t index) {
	return p[index >> 3] & (1 << (index & 7));
}

void ExplorerComponent::CollectActiveComponents(Engine& engine, Entity* entity, IExplorer& explorer) {
#ifdef _DEBUG
	assert(hostEntity == entity);
#endif
	const std::vector<Component*>& components = entity->GetComponents();
	size_t originalComponentCount = components.size();

	for (size_t i = 0; i < originalComponentCount; i++) {
		Component* component = components[i];
		if (component != nullptr) {
			explorer.Explore(i, component, false);
		}
	}
	
	for (size_t j = 0; j < collapsedComponents.size(); j++) {
		Component* component = collapsedComponents[j].component();
		assert(component != nullptr);
		explorer.Explore(originalComponentCount + j, component, true);
	}

	Bytes newState = explorer.Finalize();

	if (!newState.Empty()) {
		uint8_t* p = newState.GetData();
		size_t stateCount = newState.GetSize();
		for (size_t j = originalComponentCount; j < stateCount; j++) {
			if (CheckBit(p, j)) { // activate
				TShared<Component>& component = collapsedComponents[j - components.size()].component;
				entity->AddComponent(engine, component());
				component = nullptr;
			}
		}

		size_t k = 0;
		for (size_t i = originalComponentCount; i > 0; i--) {
			if (!CheckBit(p, i - 1)) { // deactivate
				Component* component = components[i - 1];
				while (k < collapsedComponents.size()) {
					CollapsedComponent& collapsedComponent = collapsedComponents[k];
					if (collapsedComponent.component == nullptr) {
						collapsedComponent.component = component;
						// TODO: remaining items
						break;
					}

					k++;
				}

				if (k >= collapsedComponents.size()) {
					CollapsedComponent collapsedComponent;
					collapsedComponent.component = component;
					collapsedComponents.emplace_back(collapsedComponent);
					k++;
				}

				entity->RemoveComponent(engine, component);
			}
		}

		size_t n = k;
		while (k < collapsedComponents.size()) {
			CollapsedComponent& collapsedComponent = collapsedComponents[k];
			if (collapsedComponent.component) {
				collapsedComponents[n++] = collapsedComponents[k];
			}

			k++;
		}

		collapsedComponents.resize(n);
	}
}
