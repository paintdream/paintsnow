// NavigateComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "NavigateComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class NavigateComponent;
	class NavigateComponentModule : public TReflected<NavigateComponentModule, ModuleImpl<NavigateComponent> > {
	public:
		NavigateComponentModule(Engine& engine);
		~NavigateComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<NavigateComponent> RequestNew(IScript::Request& request);
		void RequestRebuild(IScript::Request& request, IScript::Delegate<NavigateComponent> fieldComponent);
	};
}

