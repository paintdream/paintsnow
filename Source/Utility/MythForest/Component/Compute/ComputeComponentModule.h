// ComputeComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __COMPUTECOMPONENTMODULE_H__
#define __COMPUTECOMPONENTMODULE_H__

#include "ComputeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class ComputeComponent;
		class ComputeRoutine;
		class ComputeComponentModule  : public TReflected<ComputeComponentModule, ModuleImpl<ComputeComponent> > {
		public:
			ComputeComponentModule(Engine& engine);
			virtual ~ComputeComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<ComputeComponent> RequestNew(IScript::Request& request, bool transparentMode);
			TShared<ComputeRoutine> RequestLoad(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent, const String& code);
			void RequestCall(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent, IScript::Delegate<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args);
			void RequestCallAsync(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent, IScript::Request::Ref callback, IScript::Delegate<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args);
			void RequestCleanup(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent);
		};
	}
}


#endif // __COMPUTECOMPONENTMODULE_H__