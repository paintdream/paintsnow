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
			ComputeRoutine(IScript::RequestPool* pool, IScript::Request::Ref ref);
			virtual ~ComputeRoutine();
			virtual void ScriptUninitialize(IScript::Request& request) override;
			void Clear();

			IScript::RequestPool* pool;
			IScript::Request::Ref ref;
		};

		class ComputeComponent : public TAllocatedTiny<ComputeComponent, Component>, public IScript::RequestPool {
		public:
			enum {
				COMPUTECOMPONENT_TRANSPARENT = COMPONENT_CUSTOM_BEGIN,
				COMPUTECOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
			};

			ComputeComponent(Engine& engine);
			virtual ~ComputeComponent();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<ComputeRoutine> Load(const String& code);
			void Call(IScript::Request& fromRequest, TShared<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args);
			void CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, TShared<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args);

		protected:
			void Complete(IScript::RequestPool* returnPool, IScript::Request& request, IScript::Request::Ref callback, TShared<ComputeRoutine> computeRoutine);

		protected:
			Engine& engine;
		};
	}
}


#endif // __COMPUTECOMPONENT_H__