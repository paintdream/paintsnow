// ScriptComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "ScriptComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class ScriptComponent;
	class ScriptRoutine;
	class ScriptComponentModule : public TReflected<ScriptComponentModule, ModuleImpl<ScriptComponent> > {
	public:
		ScriptComponentModule(Engine& engine);
		~ScriptComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ScriptComponent> RequestNew(IScript::Request& request, const String& name);
		void RequestSetHandler(IScript::Request& request, IScript::Delegate<ScriptComponent> scriptComponent, const String& event, IScript::Request::Ref handler);

	protected:
		std::unordered_map<String, size_t> mapEventNameToID;
	};
}

