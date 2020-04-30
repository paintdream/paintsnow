// RemoteComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __REMOTECOMPONENTMODULE_H__
#define __REMOTECOMPONENTMODULE_H__

#include "RemoteComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class RemoteComponent;
		class RemoteComponentModule  : public TReflected<RemoteComponentModule, ModuleImpl<RemoteComponent> > {
		public:
			RemoteComponentModule(Engine& engine);
			virtual ~RemoteComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestRebuild(IScript::Request& request, IScript::Delegate<RemoteComponent> fieldComponent);
		};
	}
}


#endif // __REMOTECOMPONENTMODULE_H__