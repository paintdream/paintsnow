#include "FormComponentModule.h"
#include "FormComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

FormComponentModule::FormComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& FormComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestGetData)[ScriptMethod = "GetData"];
		ReflectMethod(RequestGetName)[ScriptMethod = "GetName"];
		ReflectMethod(RequestResize)[ScriptMethod = "Resize"];
		ReflectMethod(RequestSetData)[ScriptMethod = "SetData"];
	}

	return *this;
}

TShared<FormComponent> FormComponentModule::RequestNew(IScript::Request& request, const String& name) {
	CHECK_REFERENCES_NONE();

	TShared<FormComponent> formComponent = TShared<FormComponent>::From(allocator->New(name));
	formComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return formComponent;
}

void FormComponentModule::RequestResize(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(formComponent);
	CHECK_THREAD_IN_MODULE(formComponent);

	if (index >= 0) {
		formComponent->values.resize((size_t)index);
	}
}

void FormComponentModule::RequestSetData(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index, String& data) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(formComponent);
	CHECK_THREAD_IN_MODULE(formComponent);

	if (index >= 0 && index < (int64_t)formComponent->values.size()) {
		std::swap(formComponent->values[(size_t)index], data);
	}
}

void FormComponentModule::RequestGetData(IScript::Request& request, IScript::Delegate<FormComponent> formComponent, int32_t index) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(formComponent);
	CHECK_THREAD_IN_MODULE(formComponent);

	if (index >= 0 && index < (int32_t)formComponent->values.size()) {
		const String& v = formComponent->values[(size_t)index];
		engine.GetKernel().YieldCurrentWarp();
		request.DoLock();
		request << v;
		request.UnLock();
	}
}

String FormComponentModule::RequestGetName(IScript::Request& request, IScript::Delegate<FormComponent> formComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(formComponent);
	CHECK_THREAD_IN_MODULE(formComponent);

	return formComponent->name;
}