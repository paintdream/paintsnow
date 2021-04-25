#include "ExplorerComponent.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"

using namespace PaintsNow;

ExplorerComponent::ProxyConfig::ProxyConfig() : layer(0), activateThreshold(0), deactivateThreshold(0) {}

ExplorerComponent::Proxy::Proxy(Component* c) : flag(0), component(c) {}

bool ExplorerComponent::Proxy::operator < (const ExplorerComponent::Proxy& rhs) const {
	return component < rhs.component;
}

ExplorerComponent::ExplorerComponent(Unique type) : componentType(type), lastFrameIndex(0) {
	assert(!componentType->IsClass(UniqueType<ExplorerComponent>::Get()));
}

ExplorerComponent::~ExplorerComponent() {}

void ExplorerComponent::Initialize(Engine& engine, Entity* entity) {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_ACTIVATED));
	Component::Initialize(engine, entity);

	// recollect all target components
	const std::vector<Component*>& components = entity->GetComponents();
	assert(proxies.empty());

	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr && component->GetUnique()->IsClass(componentType)) {
			Proxy proxy;
			proxy.component = component;
			proxy.flag = Tiny::TINY_PINNED;
			proxies.emplace_back(proxy);
		}
	}

	std::sort(proxies.begin(), proxies.end());
}

void ExplorerComponent::SetProxyConfig(Component* component, const ProxyConfig& config) {
	std::vector<Proxy>::iterator it = std::binary_find(proxies.begin(), proxies.end(), Proxy(component));
	assert(it != proxies.end());
	it->config = config;
}

void ExplorerComponent::Uninitialize(Engine& engine, Entity* entity) {
	assert((Flag().load(std::memory_order_acquire) & TINY_ACTIVATED));
	Component::Uninitialize(engine, entity);
	proxies.clear();
}

Unique ExplorerComponent::GetExploredComponentType() const {
	return componentType;
}

void ExplorerComponent::SelectComponents(Engine& engine, Entity* entity, float refValue, std::vector<Component*, ComponentPointerAllocator>& activatedComponents) {
	uint32_t currentFrameIndex = engine.snowyStream.GetRenderResourceManager()->GetFrameIndex();
	uint32_t maxLayer = 0;
	for (size_t i = 0; i < proxies.size(); i++) {
		Proxy& proxy = proxies[i];
		assert(proxy.config.activateThreshold <= proxy.config.deactivateThreshold);
		if (refValue < proxy.config.activateThreshold) {
			maxLayer = Math::Max(maxLayer, proxy.config.layer);
		}
		
		if (refValue < proxy.config.deactivateThreshold) {
			if (!(proxy.flag & Tiny::TINY_PINNED)) {
				entity->AddComponent(engine, proxy.component());
				proxy.flag |= Tiny::TINY_PINNED;
			}
		}
	}

	// activate the max layer
	for (size_t j = 0; j < proxies.size(); j++) {
		Proxy& proxy = proxies[j];
		if (proxy.config.layer == maxLayer) {
			// activate
			proxy.flag |= Tiny::TINY_PINNED;
			activatedComponents.emplace_back(proxy.component());
		}
	}

	if (lastFrameIndex != currentFrameIndex) {
		for (size_t i = 0; i < proxies.size(); i++) {
			Proxy& proxy = proxies[i];
			if (!(proxy.flag & Tiny::TINY_PINNED)) {
				entity->RemoveComponent(engine, proxy.component());
			}

			proxy.flag &= ~Tiny::TINY_PINNED;
		}
		
		lastFrameIndex = currentFrameIndex;
	}
}

