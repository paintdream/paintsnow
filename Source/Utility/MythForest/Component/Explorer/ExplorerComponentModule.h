// ExplorerComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __EXPLORERCOMPONENTMODULE_H_
#define __EXPLORERCOMPONENTMODULE_H__

#include "ExplorerComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class ExplorerComponent;
		class ExplorerComponentModule  : public TReflected<ExplorerComponentModule, ModuleImpl<ExplorerComponent> > {
		public:
			ExplorerComponentModule(Engine& engine);
			virtual ~ExplorerComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<ExplorerComponent> RequestNew(IScript::Request& request, const String& componentType);
			void RequestSetProxyConfig(IScript::Request& request, IScript::Delegate<ExplorerComponent> explorerComponent, IScript::Delegate<Component> component, uint32_t layer, float activateThreshold, float deactivateThreshold);
		};
	}
}


#endif // __EXPLORERCOMPONENTMODULE_H__