#include "ShaderComponentModule.h"

using namespace PaintsNow;

ShaderComponentModule::ShaderComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& ShaderComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetCode)[ScriptMethod = "SetCode"];
		ReflectMethod(RequestSetInput)[ScriptMethod = "SetInput"];
		ReflectMethod(RequestSetComplete)[ScriptMethod = "SetComplete"];
	}

	return *this;
}

TShared<ShaderComponent> ShaderComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<ShaderResource> resourceTemplate) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resourceTemplate);

	TShared<ShaderComponent> terrainComponent = TShared<ShaderComponent>::From(allocator->New());
	terrainComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return terrainComponent;
}

void ShaderComponentModule::RequestSetCode(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, const String& stage, const String& text, const std::vector<std::pair<String, String> >& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderComponent);

	shaderComponent->SetCode(stage, text, config);
}

void ShaderComponentModule::RequestSetInput(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderComponent);

	shaderComponent->SetInput(stage, type, name, config);
}

void ShaderComponentModule::RequestSetComplete(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderComponent);

	shaderComponent->SetComplete();
}

