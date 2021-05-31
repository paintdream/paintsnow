#include "ExplorerComponent.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"

using namespace PaintsNow;

ExplorerComponent::Proxy::Proxy(Component* c, const ProxyConfig& conf) : component(c), config(conf) {}

bool ExplorerComponent::Proxy::operator < (const ExplorerComponent::Proxy& rhs) const {
	return component < rhs.component;
}

ExplorerComponent::ExplorerComponent(Unique id) : identifier(id) {}

ExplorerComponent::~ExplorerComponent() {}

void ExplorerComponent::SetProxyConfig(Component* component, const ProxyConfig& config) {
	std::binary_insert(proxies, Proxy(component, config));
}

Unique ExplorerComponent::GetExploreIdentifier() const {
	return identifier;
}

void ExplorerComponent::SelectComponents(Engine& engine, Entity* entity, float refValue, std::vector<Component*, ComponentPointerAllocator>& activatedComponents) const {
	const std::vector<Component*>& components = entity->GetComponents();
	activatedComponents.reserve(components.size());
	uint32_t maxLayer = 0;

	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			std::vector<Proxy>::const_iterator it = std::binary_find(proxies.begin(), proxies.end(), component);
			if (it != proxies.end()) {
				if (refValue >= it->config.activateThreshold && refValue <= it->config.deactivateThreshold) {
					activatedComponents.emplace_back(component);
				}
			} else {
				activatedComponents.emplace_back(component); // always activated for unknown components
			}
		}
	}
}

