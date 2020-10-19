#include "ShaderComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

ShaderComponent::ShaderComponent(const TShared<ShaderResource>& shader, const String& n) : name(n) {
	customMaterialShader = shader->QueryInterface(UniqueType<ShaderResourceImpl<CustomMaterialPass> >());
	assert(customMaterialShader); // by now we only support CustomMaterialPass.
	customMaterialShader = static_cast<ShaderResourceImpl<CustomMaterialPass>*>(customMaterialShader->Clone());
	std::stringstream ss;
	ss << std::hex << (void*)this;

	customMaterialShader->SetLocation(shader->GetLocation() + "/" + name + "/" + ss.str());
}

ShaderComponent::~ShaderComponent() {
	assert(!compileCallbackRef);
}

void ShaderComponent::SetCallback(IScript::Request& request, IScript::Request::Ref callback) {
	assert(Flag() & TINY_ACTIVATED);
	if (compileCallbackRef) {
		request.DoLock();
		request.Dereference(compileCallbackRef);
		request.UnLock();
	}

	compileCallbackRef = callback;
}

void ShaderComponent::Uninitialize(Engine& engine, Entity* entity) {
	if (compileCallbackRef) {
		IScript::Request& request = engine.interfaces.script.GetDefaultRequest();
		request.DoLock();
		request.Dereference(compileCallbackRef);
		request.UnLock();
	}

	BaseClass::Uninitialize(engine, entity);
}

void ShaderComponent::SetInput(Engine& engine, const String& stage, const String& type, const String& value, const String& name, const String& binding, const std::vector<std::pair<String, String> >& config) {
	assert(customMaterialShader);

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetInput(stage, type, name, value, binding, config);
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

	// Try to compile
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = engine.snowyStream.GetResourceQueue();

	ReferenceObject();
	pass.Compile(render, queue, Wrap(this, &ShaderComponent::OnShaderCompiled), &engine, customMaterialShader->GetShaderResource());
}

void ShaderComponent::OnShaderCompiled(IRender::Resource* resource, IRender::Resource::ShaderDescription& desc, IRender::Resource::ShaderDescription::Stage stage, const String& info, const String& shaderCode) {
	if (stage == IRender::Resource::ShaderDescription::END) {
		Engine* engine = reinterpret_cast<Engine*>(desc.context);
		assert(engine != nullptr);

		if (compileCallbackRef) {
			engine->bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(compileCallbackRef, info, shaderCode));
		}

#ifdef _DEBUG
		engine->interfaces.render.SetResourceNotation(resource, customMaterialShader->GetLocation());
#endif

		customMaterialShader->SetShaderResource(resource);
		if (customMaterialShader->Flag() & ResourceBase::RESOURCE_ORPHAN) {
			customMaterialShader->GetResourceManager().Insert(customMaterialShader());
		}

		ReleaseObject();
	}
}
