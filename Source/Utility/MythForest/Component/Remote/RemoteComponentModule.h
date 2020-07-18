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
		class RemoteRoutine;
		class RemoteComponentModule : public TReflected<RemoteComponentModule, ModuleImpl<RemoteComponent> > {
		public:
			RemoteComponentModule(Engine& engine);
			virtual ~RemoteComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<RemoteComponent> RequestNew(IScript::Request& request, bool transparentMode);
			TShared<RemoteRoutine> RequestLoad(IScript::Request& request, IScript::Delegate<RemoteComponent> computeComponent, const String& code);
			void RequestCall(IScript::Request& request, IScript::Delegate<RemoteComponent> computeComponent, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args);
			void RequestCallAsync(IScript::Request& request, IScript::Delegate<RemoteComponent> computeComponent, IScript::Request::Ref callback, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args);
			void RequestCleanup(IScript::Request& request, IScript::Delegate<RemoteComponent> computeComponent);
		};
	}
}


#endif // __REMOTECOMPONENTMODULE_H__