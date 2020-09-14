// ExplorerComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "ExplorerComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class ExplorerComponent;
	class ExplorerComponentModule : public TReflected<ExplorerComponentModule, ModuleImpl<ExplorerComponent> > {
	public:
		ExplorerComponentModule(Engine& engine);
		~ExplorerComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ExplorerComponent> RequestNew(IScript::Request& request, const String& componentType);
		void RequestSetProxyConfig(IScript::Request& request, IScript::Delegate<ExplorerComponent> explorerComponent, IScript::Delegate<Component> component, uint32_t layer, float activateThreshold, float deactivateThreshold);
	};
}

