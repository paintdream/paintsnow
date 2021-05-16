#include "RayTraceComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

RayTraceComponentModule::RayTraceComponentModule(Engine& engine) : BaseClass(engine) {}
RayTraceComponentModule::~RayTraceComponentModule() {}

TObject<IReflect>& RayTraceComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestGetCompletedPixelCount)[ScriptMethodLocked = "GetCompletedPixelCount"];
		ReflectMethod(RequestConfigure)[ScriptMethodLocked = "Configure"];
		ReflectMethod(RequestSetCaptureSize)[ScriptMethodLocked = "SetCaptureSize"];
		ReflectMethod(RequestGetCaptureSize)[ScriptMethodLocked = "GetCaptureSize"];
		ReflectMethod(RequestCapture)[ScriptMethodLocked = "Capture"];
		ReflectMethod(RequestGetCapturedTexture)[ScriptMethodLocked = "GetCapturedTexture"];
	}

	return *this;
}

TShared<RayTraceComponent> RayTraceComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<RayTraceComponent> rayTraceComponent = TShared<RayTraceComponent>::From(allocator->New());
	rayTraceComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return rayTraceComponent;
}

size_t RayTraceComponentModule::RequestGetCompletedPixelCount(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);

	return rayTraceComponent->GetCompletedPixelCount();
}

void RayTraceComponentModule::RequestSetCaptureSize(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, const UShort2& size) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);

	rayTraceComponent->SetCaptureSize(size);
}

void RayTraceComponentModule::RequestConfigure(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, uint16_t superSample, uint16_t tileSize, uint32_t rayCount) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);

	rayTraceComponent->Configure(superSample, tileSize, rayCount);
}

const UShort2& RayTraceComponentModule::RequestGetCaptureSize(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);

	return rayTraceComponent->GetCaptureSize();
}

void RayTraceComponentModule::RequestCapture(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, IScript::Delegate<CameraComponent> cameraComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_DELEGATE(cameraComponent);

	rayTraceComponent->Capture(engine, cameraComponent.Get());
}

TShared<TextureResource> RayTraceComponentModule::RequestGetCapturedTexture(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);

	return rayTraceComponent->GetCapturedTexture();
}

