// NavigateComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __NAVIGATECOMPONENTMODULE_H__
#define __NAVIGATECOMPONENTMODULE_H__

#include "NavigateComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class NavigateComponent;
		class NavigateComponentModule  : public TReflected<NavigateComponentModule , ModuleImpl<NavigateComponent> > {
		public:
			NavigateComponentModule(Engine& engine);
			virtual ~NavigateComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestRebuild(IScript::Request& request, IScript::Delegate<NavigateComponent> fieldComponent);
		};
	}
}


#endif // __NAVIGATECOMPONENTMODULE_H__