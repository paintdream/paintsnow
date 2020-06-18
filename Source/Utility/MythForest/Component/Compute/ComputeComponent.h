// ComputeComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __COMPUTECOMPONENT_H__
#define __COMPUTECOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ComputeRoutine : public TReflected<ComputeRoutine, SharedTiny> {
		public:
			ComputeRoutine(IScript& script, IScript::Request::Ref ref);
			virtual ~ComputeRoutine();
			virtual void ScriptUninitialize(IScript::Request& request) override;
			IScript& script;
			IScript::Request::Ref ref;
		};

		class ComputeComponent : public TAllocatedTiny<ComputeComponent, Component> {
		public:
			ComputeComponent(Engine& engine, IScript* script, IScript& hostScript);
			virtual ~ComputeComponent();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<ComputeRoutine> Load(const String& code);
			void Call(IScript::Request& fromRequest, TShared<ComputeRoutine> computeRoutine);
			void CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, TShared<ComputeRoutine> computeRoutine);
			void Cleanup();

		protected:
			void RequestSysCall(IScript::Request& request, IScript::Delegate<ComputeRoutine> computeRoutine);
			void Complete(IScript::Request& request, TShared<ComputeRoutine> computeRoutine, IScript::Request::Ref callback);

		protected:
			Engine& engine;
			IScript* script;
			IScript::Request* hostRequest;
			IScript::Request* mainRequest;
			std::map<String, IScript::Library*> library;
		};
	}
}


#endif // __COMPUTECOMPONENT_H__