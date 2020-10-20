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
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestBindRootEntity)[ScriptMethod = "BindRootEntity"];
		ReflectMethod(RequestSetPerspective)[ScriptMethod = "SetPerspective"];
		ReflectMethod(RequestGetPerspective)[ScriptMethod = "GetPerspective"];
		ReflectMethod(RequestSetVisibleDistance)[ScriptMethod = "SetVisibleDistance"];
		ReflectMethod(RequestGetVisibleDistance)[ScriptMethod = "GetVisibleDistance"];
		ReflectMethod(RequestGetCollectedEntityCount)[ScriptMethod = "GetCollectedEntityCount"];
		ReflectMethod(RequestGetCollectedVisibleEntityCount)[ScriptMethod = "GetCollectedVisibleEntityCount"];
		ReflectMethod(RequestGetCollectedTriangleCount)[ScriptMethod = "GetCollectedTriangleCount"];
		ReflectMethod(RequestSetProjectionJitter)[ScriptMethod = "SetProjectionJitter"];
		ReflectMethod(RequestSetSmoothTrack)[ScriptMethod = "SetSmoothTrack"];
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

	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << begintable
		<< key("Fov") << camera->fov
		<< key("Near") << camera->nearPlane
		<< key("Far") << camera->farPlane
		<< key("Aspect") << camera->aspect
		<< endtable;
	request.UnLock();
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
