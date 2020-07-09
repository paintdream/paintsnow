#include "MythForest.h"
#include "Component/Animation/AnimationComponentModule.h"
#include "Component/Batch/BatchComponentModule.h"
#include "Component/Cache/CacheComponentModule.h"
#include "Component/Camera/CameraComponentModule.h"
#include "Component/Compute/ComputeComponentModule.h"
#include "Component/EnvCube/EnvCubeComponentModule.h"
#include "Component/Event/EventListenerComponentModule.h"
#include "Component/Explorer/ExplorerComponentModule.h"
#include "Component/Field/FieldComponentModule.h"
#include "Component/Form/FormComponentModule.h"
#include "Component/Layout/LayoutComponentModule.h"
#include "Component/Light/LightComponentModule.h"
#include "Component/Model/ModelComponentModule.h"
#include "Component/Navigate/NavigateComponentModule.h"
#include "Component/Particle/ParticleComponentModule.h"
#include "Component/Phase/PhaseComponentModule.h"
#include "Component/Profile/ProfileComponentModule.h"
#include "Component/Remote/RemoteComponentModule.h"
#include "Component/RenderFlow/RenderFlowComponentModule.h"
#include "Component/Shape/ShapeComponentModule.h"
#include "Component/Sound/SoundComponentModule.h"
#include "Component/Space/SpaceComponentModule.h"
#include "Component/Stream/StreamComponentModule.h"
#include "Component/Surface/SurfaceComponentModule.h"
#include "Component/Terrain/TerrainComponentModule.h"
#include "Component/TextView/TextViewComponentModule.h"
#include "Component/Transform/TransformComponentModule.h"
#include "Component/Visibility/VisibilityComponentModule.h"
#include "Component/Widget/WidgetComponentModule.h"

#define USE_FRAME_CAPTURE !defined(CMAKE_PAINTSNOW) || ADD_DEBUGGER_RENDERDOC_BUILTIN

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::NsBridgeSunset;

#if USE_FRAME_CAPTURE
#include "../../General/Driver/Debugger/RenderDoc/ZDebuggerRenderDoc.h"
static ZDebuggerRenderDoc debugger;
#endif

MythForest::MythForest(Interfaces& interfaces, NsSnowyStream::SnowyStream& snowyStream, NsBridgeSunset::BridgeSunset& bridgeSunset)
	: engine(interfaces, bridgeSunset, snowyStream), lastFrameTick(0), currentFrameTime(1) {
	entityAllocator = TShared<Entity::Allocator>::From(new Entity::Allocator());

	// add builtin modules
	engine.InstallModule(new AnimationComponentModule(engine));
	engine.InstallModule(new BatchComponentModule(engine));
	engine.InstallModule(new CacheComponentModule(engine));
	engine.InstallModule(new CameraComponentModule(engine));
	engine.InstallModule(new ComputeComponentModule(engine));
	engine.InstallModule(new EnvCubeComponentModule(engine));
	engine.InstallModule(new EventListenerComponentModule(engine));
	engine.InstallModule(new ExplorerComponentModule(engine));
	engine.InstallModule(new FieldComponentModule(engine));
	engine.InstallModule(new FormComponentModule(engine));
	engine.InstallModule(new LayoutComponentModule(engine));
	engine.InstallModule(new LightComponentModule(engine));
	engine.InstallModule(new ModelComponentModule(engine, *engine.GetComponentModuleFromName("BatchComponent")->QueryInterface(UniqueType<BatchComponentModule>())));
	engine.InstallModule(new NavigateComponentModule(engine));
	engine.InstallModule(new ParticleComponentModule(engine));
	engine.InstallModule(new PhaseComponentModule(engine));
	engine.InstallModule(new ProfileComponentModule(engine));
	engine.InstallModule(new RemoteComponentModule(engine));
	engine.InstallModule(new RenderFlowComponentModule(engine));
	engine.InstallModule(new ShapeComponentModule(engine));
	engine.InstallModule(new SoundComponentModule(engine));
	engine.InstallModule(new SpaceComponentModule(engine));
	engine.InstallModule(new StreamComponentModule(engine));
	engine.InstallModule(new SurfaceComponentModule(engine));
	engine.InstallModule(new TerrainComponentModule(engine));
	engine.InstallModule(new TextViewComponentModule(engine));
	engine.InstallModule(new TransformComponentModule(engine));
	engine.InstallModule(new VisibilityComponentModule(engine));
	engine.InstallModule(new WidgetComponentModule(engine));
}

MythForest::~MythForest() {
}

void MythForest::Initialize() {
}

void MythForest::Uninitialize() {
	engine.Clear();
}

Engine& MythForest::GetEngine() {
	return engine;
}

TShared<Entity> MythForest::CreateEntity(int32_t warp) {
	TShared<Entity> entity = TShared<Entity>::From(entityAllocator->New(std::ref(engine)));
	entity->SetWarpIndex(warp < 0 ? engine.GetKernel().GetCurrentWarpIndex() : (uint32_t)warp);
	return entity;
}

TObject<IReflect>& MythForest::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(engine)[Runtime];

		unordered_map<String, Module*>& moduleMap = engine.GetModuleMap();
		for (unordered_map<String, Module*>::iterator it = moduleMap.begin(); it != moduleMap.end(); ++it) {
			*CreatePropertyWriter(reflect, this, *(*it).second, (*it).first.c_str())[ScriptLibrary = (*it).first + "Module"];
		}
	}

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestEnumerateComponentModules)[ScriptMethod = "EnumerateComponentModules"];
		ReflectMethod(RequestNewEntity)[ScriptMethod = "NewEntity"];
		ReflectMethod(RequestUpdateEntity)[ScriptMethod = "UpdateEntity"];
		ReflectMethod(RequestAddEntityComponent)[ScriptMethod = "AddEntityComponent"];
		ReflectMethod(RequestRemoveEntityComponent)[ScriptMethod = "RemoveEntityComponent"];
		ReflectMethod(RequestGetUniqueEntityComponent)[ScriptMethod = "GetUniqueEntityComponent"];
		ReflectMethod(RequestGetEntityComponents)[ScriptMethod = "GetEntityComponents"];
		ReflectMethod(RequestGetEntityComponentDetails)[ScriptMethod = "GetEntityComponentDetails"];
		ReflectMethod(RequestGetComponentType)[ScriptMethod = "GetComponentType"];
		ReflectMethod(RequestClearEntityComponents)[ScriptMethod = "ClearEntityComponents"];
		ReflectMethod(RequestGetFrameTickTime)[ScriptMethod = "GetFrameTickTime"];
		ReflectMethod(RequestRaycast)[ScriptMethod = "Raycast"];
		ReflectMethod(RequestCaptureFrame)[ScriptMethod = "CaptureFrame"];
	}

	return *this;
}

void MythForest::TickDevice(IDevice& device) {
	if (&device == &engine.interfaces.render) {
		engine.TickFrame();

		uint64_t t = ITimer::GetSystemClock();
		currentFrameTime = t - lastFrameTick;
		lastFrameTick = t;
	}
}

uint64_t MythForest::RequestGetFrameTickTime(IScript::Request& request) {
	return currentFrameTime;
}

TShared<Entity> MythForest::RequestNewEntity(IScript::Request& request, int32_t warp) {
	return CreateEntity(warp);
}

void MythForest::RequestEnumerateComponentModules(IScript::Request& request) {
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << begintable;
	const unordered_map<String, Module*>& subModules = engine.GetModuleMap();
	for (unordered_map<String, Module*>::const_iterator it = subModules.begin(); it != subModules.end(); ++it) {
		request << key((*it).first) << *((*it).second);
	}

	request << endtable;
	request.UnLock();
}

void MythForest::RequestAddEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_DELEGATE(component);
	CHECK_THREAD_IN_MODULE(entity);
	
	if (!(component->Flag() & Component::COMPONENT_LOCALIZED_WARP)) {
		CHECK_THREAD_IN_MODULE(component);
	}

	entity->AddComponent(engine, component.Get());
}

void MythForest::RequestUpdateEntity(IScript::Request& request, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	entity->UpdateEntityFlags();
	entity->UpdateBoundingBox(engine);
}

void MythForest::RequestRemoveEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_DELEGATE(component);
	CHECK_THREAD_IN_MODULE(entity);

	entity->RemoveComponent(engine, component.Get());
}

TShared<Component> MythForest::RequestGetUniqueEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, const String& componentName) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	Module* module = engine.GetComponentModuleFromName(componentName);
	if (module != nullptr) {
		Component* component = module->GetEntityUniqueComponent(entity.Get()); // Much more faster 
		if (component != nullptr) {
			return component;
		}
	}

	return nullptr;
}

void MythForest::RequestGetEntityComponentDetails(IScript::Request& request, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	std::vector<Component*> components = entity->GetComponents();

	request.DoLock();
	request << beginarray;

	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			request << begintable <<
				key("Component") << component << 
				key("Type") << component->GetUnique()->GetSubName() <<
				key("Mask") << (uint32_t)component->Flag().load(std::memory_order_relaxed) << endtable;
		}
	}

	request << endarray;
	request.UnLock();
}

void MythForest::RequestGetEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	std::vector<Component*> components = entity->GetComponents();

	request.DoLock();
	request << beginarray;

	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			request << component;
		}
	}

	request << endarray;
	request.UnLock();
}

String MythForest::RequestGetComponentType(IScript::Request& request, IScript::Delegate<Component> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(component);
	CHECK_THREAD_IN_MODULE(component);

	return component->GetUnique()->GetSubName();
}

void MythForest::RequestClearEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	entity->ClearComponents(engine);
}

class ScriptRaycastTask : public Component::RaycastTask {
public:
	ScriptRaycastTask(Engine& engine, uint32_t maxCount, IScript::Request::Ref ref) : RaycastTask(engine, maxCount), callback(ref) {}

	virtual void Finish(rvalue<std::vector<Component::RaycastResult> > r) override {
		std::vector<Component::RaycastResult>& results = r;
		engine.GetKernel().YieldCurrentWarp();
		IScript::Request& request = *engine.bridgeSunset.AllocateRequest();

		request.DoLock();
		request.Push();
		request << beginarray;
		for (uint32_t i = 0; i < results.size(); i++) {
			const Component::RaycastResult& result = results[i];
			request << begintable
				<< key("Intersection") << result.position
				<< key("Normal") << result.normal
				<< key("TexCoord") << result.coord
				<< key("Distance") << result.distance
				<< key("Object") << result.unit
				<< key("Parent") << result.parent
				<< endtable;
		}
		request << endarray;
		request.Call(sync, callback);
		request.Pop();
		request.Dereference(callback);
		request.UnLock();

		engine.bridgeSunset.FreeRequest(&request);
	}

protected:
	IScript::Request::Ref callback;
};

void MythForest::RequestRaycast(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Request::Ref callback, const Float3& from, const Float3& dir, uint32_t count) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	TShared<ScriptRaycastTask> task = TShared<ScriptRaycastTask>::From(new ScriptRaycastTask(engine, count, callback));
	task->AddPendingTask();
	Float3Pair ray(from, dir);
	Component::RaycastForEntity(*task(), ray, entity.Get());
	task->RemovePendingTask();
}

void MythForest::RequestCaptureFrame(IScript::Request& request, const String& path, const String& options) {
	InvokeCaptureFrame(path, options);
}

//
void MythForest::OnSize(const Int2& size) {
	EventListenerComponentModule& eventListenerComponentModule = *engine.GetComponentModuleFromName("EventListenerComponent")->QueryInterface(UniqueType<EventListenerComponentModule>());
	eventListenerComponentModule.OnSize(size);
}

void MythForest::OnMouse(const IFrame::EventMouse& mouse) {
	EventListenerComponentModule& eventListenerComponentModule = *engine.GetComponentModuleFromName("EventListenerComponent")->QueryInterface(UniqueType<EventListenerComponentModule>());
	eventListenerComponentModule.OnMouse(mouse);
}

void MythForest::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	EventListenerComponentModule& eventListenerComponentModule = *engine.GetComponentModuleFromName("EventListenerComponent")->QueryInterface(UniqueType<EventListenerComponentModule>());
	eventListenerComponentModule.OnKeyboard(keyboard);
}

void MythForest::StartCaptureFrame(const String& path, const String& options) {
#if USE_FRAME_CAPTURE
	debugger.SetDumpHandler(path, TWrapper<bool>());
	debugger.StartDump(options);
#endif
}

void MythForest::EndCaptureFrame() {
#if USE_FRAME_CAPTURE
	debugger.EndDump();
#endif
}

void MythForest::InvokeCaptureFrame(const String& path, const String& options) {
#if USE_FRAME_CAPTURE
	debugger.SetDumpHandler(path, TWrapper<bool>());
	debugger.InvokeDump(options);
#endif
}
