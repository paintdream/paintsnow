#include "MythForest.h"
#include "Component/Animation/AnimationComponentModule.h"
#include "Component/Batch/BatchComponentModule.h"
#include "Component/Cache/CacheComponentModule.h"
#include "Component/Camera/CameraComponentModule.h"
#include "Component/EnvCube/EnvCubeComponentModule.h"
#include "Component/Event/EventComponentModule.h"
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
#include "Component/Script/ScriptComponentModule.h"
#include "Component/Shader/ShaderComponentModule.h"
#include "Component/Shape/ShapeComponentModule.h"
#include "Component/Sound/SoundComponentModule.h"
#include "Component/Space/SpaceComponentModule.h"
#include "Component/Stream/StreamComponentModule.h"
#include "Component/Surface/SurfaceComponentModule.h"
#include "Component/Tape/TapeComponentModule.h"
#include "Component/Terrain/TerrainComponentModule.h"
#include "Component/TextView/TextViewComponentModule.h"
#include "Component/Transform/TransformComponentModule.h"
#include "Component/Visibility/VisibilityComponentModule.h"
#include "Component/Widget/WidgetComponentModule.h"

#define USE_FRAME_CAPTURE !CMAKE_PAINTSNOW || ADD_DEBUGGER_RENDERDOC_BUILTIN

using namespace PaintsNow;

#if USE_FRAME_CAPTURE
#include "../../General/Driver/Debugger/RenderDoc/ZDebuggerRenderDoc.h"
static ZDebuggerRenderDoc debugger;
#endif

MythForest::MythForest(Interfaces& interfaces, SnowyStream& snowyStream, BridgeSunset& bridgeSunset)
	: engine(interfaces, bridgeSunset, snowyStream), lastFrameTick(0), currentFrameTime(1) {
	entityAllocator = TShared<Entity::Allocator>::From(new Entity::Allocator());
}

MythForest::~MythForest() {
}

void MythForest::Initialize() {
	// add builtin modules
	engine.InstallModule(new AnimationComponentModule(engine));
	engine.InstallModule(new BatchComponentModule(engine));
	engine.InstallModule(new CacheComponentModule(engine));
	engine.InstallModule(new CameraComponentModule(engine));
	engine.InstallModule(new EnvCubeComponentModule(engine));
	engine.InstallModule(new EventComponentModule(engine));
	engine.InstallModule(new ExplorerComponentModule(engine));
	engine.InstallModule(new FieldComponentModule(engine));
	engine.InstallModule(new FormComponentModule(engine));
	engine.InstallModule(new LayoutComponentModule(engine));
	engine.InstallModule(new LightComponentModule(engine));
	engine.InstallModule(new ModelComponentModule(engine));
	engine.InstallModule(new NavigateComponentModule(engine));
	engine.InstallModule(new ParticleComponentModule(engine));
	engine.InstallModule(new PhaseComponentModule(engine));
	engine.InstallModule(new ProfileComponentModule(engine));
	engine.InstallModule(new RemoteComponentModule(engine));
	engine.InstallModule(new RenderFlowComponentModule(engine));
	engine.InstallModule(new ScriptComponentModule(engine));
	engine.InstallModule(new ShaderComponentModule(engine));
	engine.InstallModule(new ShapeComponentModule(engine));
	engine.InstallModule(new SoundComponentModule(engine));
	engine.InstallModule(new SpaceComponentModule(engine));
	engine.InstallModule(new StreamComponentModule(engine));
	engine.InstallModule(new SurfaceComponentModule(engine));
	engine.InstallModule(new TapeComponentModule(engine));
	engine.InstallModule(new TerrainComponentModule(engine));
	engine.InstallModule(new TextViewComponentModule(engine));
	engine.InstallModule(new TransformComponentModule(engine));
	engine.InstallModule(new VisibilityComponentModule(engine));
	engine.InstallModule(new WidgetComponentModule(engine));
}

void MythForest::Uninitialize() {
	IScript::Request& request = engine.bridgeSunset.GetScript().GetDefaultRequest();
	request.DoLock();
	while (!nextFrameListeners.Empty()) {
		request.Dereference(nextFrameListeners.Top().second);
		nextFrameListeners.Pop();
	}
	request.UnLock();

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

		std::unordered_map<String, Module*>& moduleMap = engine.GetModuleMap();
		for (std::unordered_map<String, Module*>::iterator it = moduleMap.begin(); it != moduleMap.end(); ++it) {
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
		ReflectMethod(RequestGetComponentType)[ScriptMethod = "GetComponentType"];
		ReflectMethod(RequestClearEntityComponents)[ScriptMethod = "ClearEntityComponents"];
		ReflectMethod(RequestGetFrameTickTime)[ScriptMethod = "GetFrameTickTime"];
		ReflectMethod(RequestWaitForNextFrame)[ScriptMethod = "WaitForNextFrame"];
		ReflectMethod(RequestRaycast)[ScriptMethod = "Raycast"];
		ReflectMethod(RequestCaptureFrame)[ScriptMethod = "CaptureFrame"];
	}

	return *this;
}

void MythForest::TickDevice(IDevice& device) {
	if (&device == &engine.interfaces.render) {
		std::vector<std::pair<TShared<Entity>, IScript::Request::Ref> > listeners;

		while (!nextFrameListeners.Empty()) {
			std::pair<TShared<Entity>, IScript::Request::Ref>& entry = nextFrameListeners.Top();
			listeners.emplace_back(std::move(entry));
			entry.first = nullptr;
			nextFrameListeners.Pop();
		}

		engine.TickFrame();

		uint64_t t = ITimer::GetSystemClock();
		currentFrameTime = t - lastFrameTick;
		lastFrameTick = t;

		for (size_t k = 0; k < listeners.size(); k++) {
			std::pair<TShared<Entity>, IScript::Request::Ref>& entry = listeners[k];
			engine.GetKernel().QueueRoutine(entry.first(), CreateTaskScriptOnce(entry.second));
		}
	}
}

uint64_t MythForest::RequestGetFrameTickTime(IScript::Request& request) {
	return currentFrameTime;
}

void MythForest::RequestWaitForNextFrame(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);

	request.DoLock();
	nextFrameListeners.Push(std::make_pair(entity.Get(), callback));
	request.UnLock();
}

TShared<Entity> MythForest::RequestNewEntity(IScript::Request& request, int32_t warp) {
	return CreateEntity(warp);
}

void MythForest::RequestEnumerateComponentModules(IScript::Request& request) {
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << begintable;
	const std::unordered_map<String, Module*>& subModules = engine.GetModuleMap();
	for (std::unordered_map<String, Module*>::const_iterator it = subModules.begin(); it != subModules.end(); ++it) {
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
	
	if (!(component->Flag().load(std::memory_order_acquire) & Component::COMPONENT_LOCALIZED_WARP)) {
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
	} else {
		const std::vector<Component*>& components = entity->GetComponents();
		for (size_t i = 0; i < components.size(); i++) {
			Component* component = components[i];
			if (component != nullptr && (component->Flag().load(std::memory_order_acquire) & Component::COMPONENT_ALIASED_TYPE)) {
				if (component->GetAliasedTypeName() == componentName) {
					return component;
				}
			}
		}
	}

	return nullptr;
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

	if (component->Flag().load(std::memory_order_acquire) & Component::COMPONENT_ALIASED_TYPE) {
		return component->GetAliasedTypeName();
	} else {
		return component->GetUnique()->GetBriefName();
	}
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

	void Finish(rvalue<std::vector<Component::RaycastResult> > r) override {
		std::vector<Component::RaycastResult>& results = r;
		engine.GetKernel().YieldCurrentWarp();
		IScript::Request& request = *engine.bridgeSunset.AcquireSafe();

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

		engine.bridgeSunset.ReleaseSafe(&request);
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
	EventComponentModule& eventComponentModule = *engine.GetComponentModuleFromName("EventComponent")->QueryInterface(UniqueType<EventComponentModule>());
	eventComponentModule.OnSize(size);
}

void MythForest::OnMouse(const IFrame::EventMouse& mouse) {
	EventComponentModule& eventComponentModule = *engine.GetComponentModuleFromName("EventComponent")->QueryInterface(UniqueType<EventComponentModule>());
	eventComponentModule.OnMouse(mouse);
}

void MythForest::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	EventComponentModule& eventComponentModule = *engine.GetComponentModuleFromName("EventComponent")->QueryInterface(UniqueType<EventComponentModule>());
	eventComponentModule.OnKeyboard(keyboard);
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
