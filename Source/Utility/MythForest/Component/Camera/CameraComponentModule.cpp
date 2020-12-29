#include "CameraComponentModule.h"
#include "CameraComponent.h"
#include "../../Entity.h"
#include "../RenderFlow/RenderFlowComponent.h"
#include "../Event/EventComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CameraComponentModule::CameraComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& CameraComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestBindRootEntity)[ScriptMethodLocked = "BindRootEntity"];
		ReflectMethod(RequestSetPerspective)[ScriptMethodLocked = "SetPerspective"];
		ReflectMethod(RequestGetPerspective)[ScriptMethodLocked = "GetPerspective"];
		ReflectMethod(RequestSetVisibleDistance)[ScriptMethodLocked = "SetVisibleDistance"];
		ReflectMethod(RequestGetVisibleDistance)[ScriptMethodLocked = "GetVisibleDistance"];
		ReflectMethod(RequestGetCollectedEntityCount)[ScriptMethodLocked = "GetCollectedEntityCount"];
		ReflectMethod(RequestGetCollectedVisibleEntityCount)[ScriptMethodLocked = "GetCollectedVisibleEntityCount"];
		ReflectMethod(RequestGetCollectedTriangleCount)[ScriptMethodLocked = "GetCollectedTriangleCount"];
		ReflectMethod(RequestSetProjectionJitter)[ScriptMethodLocked = "SetProjectionJitter"];
		ReflectMethod(RequestSetSmoothTrack)[ScriptMethodLocked = "SetSmoothTrack"];
	}

	return *this;
}

// Interfaces
TShared<CameraComponent> CameraComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& cameraRenderPortName) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(renderFlowComponent);

	RenderFlowComponent* flow = renderFlowComponent.Get();
	TShared<CameraComponent> cameraComponent = TShared<CameraComponent>::From(allocator->New(flow, cameraRenderPortName));
	cameraComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return cameraComponent;
}

uint32_t CameraComponentModule::RequestGetCollectedEntityCount(IScript::Request& request, IScript::Delegate<CameraComponent> camera) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	return camera->GetCollectedEntityCount();
}

uint32_t CameraComponentModule::RequestGetCollectedTriangleCount(IScript::Request& request, IScript::Delegate<CameraComponent> camera) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	return camera->GetCollectedTriangleCount();
}

uint32_t CameraComponentModule::RequestGetCollectedVisibleEntityCount(IScript::Request& request, IScript::Delegate<CameraComponent> camera) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	return camera->GetCollectedVisibleEntityCount();
}

void CameraComponentModule::RequestBindRootEntity(IScript::Request& request, IScript::Delegate<CameraComponent> camera, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(camera);

	camera->BindRootEntity(engine, entity.Get());
}

void CameraComponentModule::RequestSetPerspective(IScript::Request& request, IScript::Delegate<CameraComponent> camera, float d, float n, float f, float r) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	camera->fov = d;
	camera->nearPlane = n;
	camera->farPlane = f;
	camera->aspect = r;
}

void CameraComponentModule::RequestGetPerspective(IScript::Request& request, IScript::Delegate<CameraComponent> camera) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	request << begintable
		<< key("Fov") << camera->fov
		<< key("Near") << camera->nearPlane
		<< key("Far") << camera->farPlane
		<< key("Aspect") << camera->aspect
		<< endtable;
}

void CameraComponentModule::RequestSetVisibleDistance(IScript::Request& request, IScript::Delegate<CameraComponent> camera, float distance) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	camera->viewDistance = distance;
}

float CameraComponentModule::RequestGetVisibleDistance(IScript::Request& request, IScript::Delegate<CameraComponent> camera) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	return camera->viewDistance;
}

void CameraComponentModule::RequestSetProjectionJitter(IScript::Request& request, IScript::Delegate<CameraComponent> camera, bool jitter) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	if (jitter) {
		camera->Flag().fetch_or(CameraComponent::CAMERACOMPONENT_SUBPIXEL_JITTER, std::memory_order_relaxed);
	} else {
		camera->Flag().fetch_and(~CameraComponent::CAMERACOMPONENT_SUBPIXEL_JITTER, std::memory_order_relaxed);
	}
}

void CameraComponentModule::RequestSetSmoothTrack(IScript::Request& request, IScript::Delegate<CameraComponent> camera, bool smoothTrack) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(camera);
	CHECK_THREAD_IN_MODULE(camera);

	if (smoothTrack) {
		camera->Flag().fetch_or(CameraComponent::CAMERACOMPONENT_SMOOTH_TRACK, std::memory_order_relaxed);
	} else {
		camera->Flag().fetch_and(~CameraComponent::CAMERACOMPONENT_SMOOTH_TRACK, std::memory_order_relaxed);
	}
}
