#include "TransformComponentModule.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

TransformComponentModule::TransformComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& TransformComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];

		ReflectMethod(RequestEditorRotate)[ScriptMethod = "EditorRotate"];
		ReflectMethod(RequestSetTranslation)[ScriptMethod = "SetTranslation"];
		ReflectMethod(RequestGetTranslation)[ScriptMethod = "GetTranslation"];
		ReflectMethod(RequestGetQuickTranslation)[ScriptMethod = "GetQuickTranslation"];
		ReflectMethod(RequestSetRotation)[ScriptMethod = "SetRotation"];
		ReflectMethod(RequestGetRotation)[ScriptMethod = "GetRotation"];
		ReflectMethod(RequestSetScale)[ScriptMethod = "SetScale"];
		ReflectMethod(RequestGetScale)[ScriptMethod = "GetScale"];
		ReflectMethod(RequestGetAxises)[ScriptMethod = "GetAxises"];
		ReflectMethod(RequestUpdateTransform)[ScriptMethod = "UpdateTransform"];
	}

	return *this;
}

TShared<TransformComponent> TransformComponentModule::RequestNew(IScript::Request& request) {
	TShared<TransformComponent> transformComponent = TShared<TransformComponent>::From(allocator->New());
	transformComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return transformComponent;
}

Float3 TransformComponentModule::RequestGetQuickTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	return transformComponent->GetQuickTranslation();
}

void TransformComponentModule::RequestEditorRotate(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float2& from, Float2& to) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->EditorRotate(from, to);
}

void TransformComponentModule::RequestSetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& rotation) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->SetRotation(rotation);
}

Float3 TransformComponentModule::RequestGetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);
	return transformComponent->GetRotation();
}

void TransformComponentModule::RequestSetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& scale) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->SetScale(scale);
}

Float3 TransformComponentModule::RequestGetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	return transformComponent->GetScale();
}

void TransformComponentModule::RequestSetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& translation) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->SetTranslation(translation);
}

Float3 TransformComponentModule::RequestGetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	return transformComponent->GetTranslation();
}

void TransformComponentModule::RequestGetAxises(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	Float3 x, y, z;
	transformComponent->GetAxises(x, y, z);

	request.DoLock();
	request << beginarray << x << y << z << endarray;
	request.UnLock();
}

void TransformComponentModule::RequestUpdateTransform(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(transformComponent);
	CHECK_THREAD_IN_MODULE(transformComponent);

	transformComponent->UpdateTransform();
}
