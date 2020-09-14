// FormComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "FormComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class FormComponent;
	class FormComponentModule : public TReflected<FormComponentModule, ModuleImpl<FormComponent> > {
	public:
		FormComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<FormComponent> RequestNew(IScript::Request& request, const String& name);
		void RequestResize(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index);
		void RequestSetData(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index, String& data);
		void RequestGetData(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index);
		String RequestGetName(IScript::Request& request, IScript::Delegate<FormComponent> formComponent);
	};
}

