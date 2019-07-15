// FieldComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __FIELDCOMPONENTMODULE_H__
#define __FIELDCOMPONENTMODULE_H__

#include "FieldComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class FieldComponent;
		class FieldComponentModule  : public TReflected<FieldComponentModule , ModuleImpl<FieldComponent> > {
		public:
			FieldComponentModule(Engine& engine);
			virtual ~FieldComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestRebuild(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent);
		};
	}
}


#endif // __FIELDCOMPONENTMODULE_H__