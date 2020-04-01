// BatchComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __BATCHCOMPONENTMODULE_H__
#define __BATCHCOMPONENTMODULE_H__

#include "BatchComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class BatchComponentModule  : public TReflected<BatchComponentModule , ModuleImpl<BatchComponent> > {
		public:
			BatchComponentModule(Engine& engine);

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			TShared<BatchComponent> Create();

		public:
			// APIs
			void RequestNew(IScript::Request& request);
			void RequestGetCaptureStatistics(IScript::Request& request, IScript::Delegate<BatchComponent> component);
		};
	}
}


#endif // __BATCHCOMPONENTMODULE_H__
