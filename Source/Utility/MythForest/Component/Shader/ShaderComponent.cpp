#include "ShaderComponent.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;

ShaderComponent::ShaderComponent() : compilingShaderResource(nullptr) {}

void ShaderComponent::SetCallback(IScript::Request& request, IScript::Request::Ref callback) {
	assert(Flag() & TINY_ACTIVATED);
	if (compileCallbackRef) {
		request.DoLock();
		request.Dereference(compileCallbackRef);
		request.UnLock();
	}

	compileCallbackRef = callback;
}

void ShaderComponent::Initialize(Engine& engine, Entity* entity) {
	TShared<ShaderResource> shaderResource = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), "[Runtime]/ShaderResource/CustomMaterialPass");
	assert(shaderResource);
	customMaterialShader = shaderResource->QueryInterface(UniqueType<ShaderResourceImpl<CustomMaterialPass> >());
	assert(customMaterialShader);
	customMaterialShader = static_cast<ShaderResourceImpl<CustomMaterialPass>*>(customMaterialShader->Clone());

	BaseClass::Initialize(engine, entity);
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

void ShaderComponent::SetInput(Engine& engine, const String& stage, const String& type, const String& value, const String& name, const std::vector<std::pair<String, String> >& config) {
	assert(customMaterialShader);

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetInput(stage, type, name, value, config);
}

void ShaderComponent::SetCode(Engine& engine, const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) {
	assert(customMaterialShader);

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetCode(stage, code, config);
}

void ShaderComponent::SetComplete(Engine& engine) {
	assert(customMaterialShader);
	std::atomic_thread_fence(std::memory_order_acquire);
	if (compilingShaderResource != nullptr)
		return;

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetComplete();

	// Try to compile
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = engine.snowyStream.GetResourceQueue();

	ReferenceObject();
	compilingShaderResource = pass.Compile(render, queue, Wrap(this, &ShaderComponent::OnShaderCompiled));
}

void ShaderComponent::OnShaderCompiled(IRender::Resource::ShaderDescription&, IRender::Resource::ShaderDescription::Stage stage, const String& info, const String& shaderCode) {
	if (compileCallbackRef) {
		// TODO:
	}

	if (stage == IRender::Resource::ShaderDescription::END) {
		if (info.empty()) { // success?
			customMaterialShader->SetShaderResource(compilingShaderResource);
		} else {
			DeviceResourceManager<IRender>& manager = static_cast<DeviceResourceManager<IRender>&>(customMaterialShader->GetResourceManager());
			manager.GetDevice().DeleteResource(static_cast<IRender::Queue*>(manager.GetContext()), compilingShaderResource);
		}

		compilingShaderResource = nullptr;
		std::atomic_thread_fence(std::memory_order_release);
		ReleaseObject();
	}
}
