// ExplorerComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Space/SpaceComponent.h"

namespace PaintsNow {
	// Level of details controller
	class ExplorerComponent : public TAllocatedTiny<ExplorerComponent, Component> {
	public:
		ExplorerComponent(Unique componentType);
		virtual ~ExplorerComponent();

		virtual void Initialize(Engine& engine, Entity* entity);
		virtual void Uninitialize(Engine& engine, Entity* entity);

		struct ProxyConfig {
			ProxyConfig();
			uint32_t layer;
			float activateThreshold;
			float deactivateThreshold;
		};

		void SetProxyConfig(Component* component, const ProxyConfig& config);
		Unique GetExploredComponentType() const;
		void SelectComponents(Engine& engine, Entity* entity, float refValue, std::vector<Component*>& collectedComponents);

	protected:
		struct Proxy {
			Proxy(Component* c = nullptr);
			operator Component* () const {
				return component();
			}

			bool operator < (const Proxy& rhs) const;
			TShared<Component> component;
			Tiny::FLAG flag;
			ProxyConfig config;
		};

		std::vector<Proxy> proxies;
		Unique componentType;
		uint32_t lastFrameIndex;
	};
}

