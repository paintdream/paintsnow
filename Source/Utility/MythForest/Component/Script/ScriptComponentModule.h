// ScriptComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __SCRIPTCOMPONENTMODULE_H__
#define __SCRIPTCOMPONENTMODULE_H__

#include "ScriptComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class ScriptComponent;
		class ScriptRoutine;
		class ScriptComponentModule : public TReflected<ScriptComponentModule, ModuleImpl<ScriptComponent> > {
		public:
			ScriptComponentModule(Engine& engine);
			virtual ~ScriptComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<ScriptComponent> RequestNew(IScript::Request& request);
			void RequestSetHandler(IScript::Request& request, IScript::Delegate<ScriptComponent> scriptComponent, const String& event, IScript::Request::Ref handler);

		protected:
			unordered_map<String, size_t> mapEventNameToID;
		};
	}
}


#endif // __SCRIPTCOMPONENTMODULE_H__