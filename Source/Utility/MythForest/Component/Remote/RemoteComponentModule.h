// RemoteComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "RemoteComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class RemoteComponent;
	class RemoteRoutine;
	class RemoteComponentModule : public TReflected<RemoteComponentModule, ModuleImpl<RemoteComponent> > {
	public:
		RemoteComponentModule(Engine& engine);
		~RemoteComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<RemoteComponent> RequestNew(IScript::Request& request, bool transparentMode);
		TShared<RemoteRoutine> RequestLoad(IScript::Request& request, IScript::Delegate<RemoteComponent> computeComponent, const String& code);
		void RequestCall(IScript::Request& request, IScript::Delegate<RemoteComponent> computeComponent, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args);
		void RequestCallAsync(IScript::Request& request, IScript::Delegate<RemoteComponent> computeComponent, IScript::Request::Ref callback, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args);
		void RequestCleanup(IScript::Request& request, IScript::Delegate<RemoteComponent> computeComponent);
	};
}

