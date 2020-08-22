#include "LightComponentModule.h"

using namespace PaintsNow;

LightComponentModule::LightComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& LightComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetLightColor)[ScriptMethod = "SetLightColor"];
		ReflectMethod(RequestSetLightAttenuation)[ScriptMethod = "SetLightAttenuation"];
		ReflectMethod(RequestSetLightDirectional)[ScriptMethod = "SetLightDirectional"];
		ReflectMethod(RequestSetLightRange)[ScriptMethod = "SetLightRange"];
		ReflectMethod(RequestBindLightShadowStream)[ScriptMethod = "BindLightShadowStream"];
		// ReflectMethod(RequestSetLightSpotAngle)[ScriptMethod = "SetLightSpotAngle"];
	}

	return *this;
}

TShared<LightComponent> LightComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<LightComponent> lightComponent = TShared<LightComponent>::From(allocator->New());
	lightComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return lightComponent;
}

void LightComponentModule::RequestSetLightAttenuation(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, float attenuation) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(lightComponent);
	CHECK_THREAD_IN_MODULE(lightComponent);

	lightComponent->SetAttenuation(attenuation);
}

/*
void LightComponentModule::RequestSetLightSpotAngle(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, float spotAngle) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(lightComponent);
	CHECK_THREAD_IN_MODULE(lightComponent);

	lightComponent->spotAngle = spotAngle;
}*/

void LightComponentModule::RequestSetLightColor(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, Float3& color) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(lightComponent);
	CHECK_THREAD_IN_MODULE(lightComponent);

	lightComponent->SetColor(color);
}

void LightComponentModule::RequestSetLightDirectional(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, bool directional) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(lightComponent);
	CHECK_THREAD_IN_MODULE(lightComponent);

	if (directional) {
		lightComponent->Flag().fetch_or(LightComponent::LIGHTCOMPONENT_DIRECTIONAL, std::memory_order_acquire);
	} else {
		lightComponent->Flag().fetch_and(~LightComponent::LIGHTCOMPONENT_DIRECTIONAL, std::memory_order_release);
	}
}

void LightComponentModule::RequestSetLightRange(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, Float3& range) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(lightComponent);
	CHECK_THREAD_IN_MODULE(lightComponent);

	lightComponent->SetRange(range);
}

void LightComponentModule::RequestBindLightShadowStream(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, uint32_t layer, IScript::Delegate<StreamComponent> streamComponent, const UShort2& res, float gridSize, float scale) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(lightComponent);
	CHECK_DELEGATE(streamComponent);
	CHECK_THREAD_IN_MODULE(lightComponent);
	CHECK_THREAD_IN_MODULE(streamComponent);

	lightComponent->BindShadowStream(engine, layer, streamComponent.Get(), res, gridSize, scale);
}
