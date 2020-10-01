#include "ShaderComponent.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;

ShaderComponent::ShaderComponent() {}

void ShaderComponent::Initialize(Engine& engine, Entity* entity) {
	TShared<ShaderResource> shaderResource = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), "[Runtime]/ShaderResource/CustomMaterialPass");
	assert(shaderResource);
	customMaterialShader = shaderResource->QueryInterface(UniqueType<ShaderResourceImpl<CustomMaterialPass> >());
	assert(customMaterialShader);
	customMaterialShader = static_cast<ShaderResourceImpl<CustomMaterialPass>*>(customMaterialShader->Clone());

	BaseClass::Initialize(engine, entity);
}

void ShaderComponent::Uninitialize(Engine& engine, Entity* entity) {
	BaseClass::Uninitialize(engine, entity);
}

void ShaderComponent::SetInput(Engine& engine, const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config) {
	assert(customMaterialShader);

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetInput(stage, type, name, config);
}

void ShaderComponent::SetCode(Engine& engine, const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) {
	assert(customMaterialShader);

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetCode(stage, code, config);
}

void ShaderComponent::SetComplete(Engine& engine) {
	assert(customMaterialShader);

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetComplete();
}
