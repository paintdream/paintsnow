#include "MythForest.h"
#define USE_FRAME_CAPTURE !defined(CMAKE_PAINTSNOW) || ADD_DEBUGGER_RENDERDOC_BUILTIN

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::NsBridgeSunset;

#if USE_FRAME_CAPTURE
#include "../../General/Driver/Debugger/RenderDoc/ZDebuggerRenderDoc.h"
static ZDebuggerRenderDoc debugger;
#endif


class ModuleRegistar : public IReflect {
public:
	ModuleRegistar(Engine& e) : engine(e), IReflect(true, false) {}
	virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
		if (!s.IsBasicObject() && s.QueryInterface(UniqueType<Module>()) != nullptr) {
			Module& module = static_cast<Module&>(s);
			engine.InstallModule(&module);
		}
	}
	virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

protected:
	Engine& engine;
};

MythForest::MythForest(Interfaces& interfaces, NsSnowyStream::SnowyStream& snowyStream, NsBridgeSunset::BridgeSunset& bridgeSunset)
	: engine(interfaces, bridgeSunset, snowyStream, *this), lastFrameTick(0), currentFrameTime(1),
	animationComponentModule(engine),
	batchComponentModule(engine),
	envCubeComponentModule(engine),
	eventListenerComponentModule(engine),
	explorerComponentModule(engine),
	cameraComponentModule(engine),
	computeComponentModule(engine),
	fieldComponentModule(engine),
	formComponentModule(engine),
	modelComponentModule(engine, batchComponentModule),
	navigateComponentModule(engine),
	layoutComponentModule(engine),
	lightComponentModule(engine),
	particleComponentModule(engine),
	phaseComponentModule(engine),
	profileComponentModule(engine),
	remoteComponentModule(engine),
	renderFlowComponentModule(engine),
	shapeComponentModule(engine),
	soundComponentModule(engine),
	spaceComponentModule(engine),
	surfaceComponentModule(engine),
	terrainComponentModule(engine),
	textViewComponentModule(engine),
	transformComponentModule(engine),
	visibilityComponentModule(engine),
	widgetComponentModule(engine) {
	entityAllocator = TShared<Entity::Allocator>::From(new Entity::Allocator());
}

MythForest::~MythForest() {
}

void MythForest::Initialize() {
	ModuleRegistar registar(engine);
	(*this)(registar);
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
	
		ReflectProperty(animationComponentModule)[ScriptLibrary = "AnimationComponentModule"];
		ReflectProperty(batchComponentModule)[ScriptLibrary = "BatchComponentModule"];
		ReflectProperty(cameraComponentModule)[ScriptLibrary = "CameraComponentModule"];
		ReflectProperty(computeComponentModule)[ScriptLibrary = "ComputeComponentModule"];
		ReflectProperty(envCubeComponentModule)[ScriptLibrary = "EnvCubeComponentModule"];
		ReflectProperty(eventListenerComponentModule)[ScriptLibrary = "EventListenerComponentModule"];
		ReflectProperty(explorerComponentModule)[ScriptLibrary = "ExplorerComponentModule"];
		ReflectProperty(fieldComponentModule)[ScriptLibrary = "FieldComponentModule"];
		ReflectProperty(formComponentModule)[ScriptLibrary = "FormComponentModule"];
		ReflectProperty(layoutComponentModule)[ScriptLibrary = "LayoutComponentModule"];
		ReflectProperty(lightComponentModule)[ScriptLibrary = "LightComponentModule"];
		ReflectProperty(modelComponentModule)[ScriptLibrary = "ModelComponentModule"];
		ReflectProperty(navigateComponentModule)[ScriptLibrary = "NavigateComponentModule"];
		ReflectProperty(particleComponentModule)[ScriptLibrary = "ParticleComponentModule"];
		ReflectProperty(phaseComponentModule)[ScriptLibrary = "PhaseComponentModule"];
		ReflectProperty(profileComponentModule)[ScriptLibrary = "ProfileComponentModule"];
		ReflectProperty(remoteComponentModule)[ScriptLibrary = "RemoteComponentModule"];
		ReflectProperty(renderFlowComponentModule)[ScriptLibrary = "RenderFlowComponentModule"];
		ReflectProperty(shapeComponentModule)[ScriptLibrary = "ShapeComponentModule"];
		ReflectProperty(soundComponentModule)[ScriptLibrary = "SoundComponentModule"];
		ReflectProperty(spaceComponentModule)[ScriptLibrary = "SpaceComponentModule"];
		ReflectProperty(surfaceComponentModule)[ScriptLibrary = "SurfaceComponentModule"];
		ReflectProperty(terrainComponentModule)[ScriptLibrary = "TerrainComponentModule"];
		ReflectProperty(textViewComponentModule)[ScriptLibrary = "TextViewComponentModule"];
		ReflectProperty(transformComponentModule)[ScriptLibrary = "TransformComponentModule"];
		ReflectProperty(visibilityComponentModule)[ScriptLibrary = "VisibilityComponentModule"];
		ReflectProperty(widgetComponentModule)[ScriptLibrary = "WidgetComponentModule"];
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

void MythForest::RequestGetFrameTickTime(IScript::Request& request) {
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << currentFrameTime;
	request.UnLock();
}

void MythForest::RequestNewEntity(IScript::Request& request, int32_t warp) {
	TShared<Entity> entity = CreateEntity(warp);

	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << entity;
	request.UnLock();
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

void MythForest::RequestGetUniqueEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, const String& componentName) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	Module* module = engine.GetComponentModuleFromName(componentName);
	if (module != nullptr) {
		Component* component = module->GetEntityUniqueComponent(entity.Get()); // Much more faster 
		if (component != nullptr) {
			engine.GetKernel().YieldCurrentWarp();
			request.DoLock();
			request << component;
			request.UnLock();
		}
	}
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

void MythForest::RequestGetComponentType(IScript::Request& request, IScript::Delegate<Component> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(component);
	CHECK_THREAD_IN_MODULE(component);

	request.DoLock();
	request << component->GetUnique()->GetSubName();
	request.UnLock();
}

void MythForest::RequestClearEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	entity->ClearComponents(engine);
}

void MythForest::RequestRaycast(IScript::Request& request, IScript::Delegate<Entity> entity, const Float3& from, const Float3& dir, uint32_t count) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	std::vector<Unit::RaycastResult> results;
	Float3Pair ray(from, dir);
	entity->Raycast(results, ray, count, nullptr);
	if (!results.empty()) {
		engine.GetKernel().YieldCurrentWarp();

		request.DoLock();
		request << beginarray;
		for (uint32_t i = 0; i < results.size(); i++) {
			const Unit::RaycastResult& result = results[i];
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
		request.UnLock();
	}
}

void MythForest::RequestCaptureFrame(IScript::Request& request, const String& path, const String& options) {
	InvokeCaptureFrame(path, options);
}

//
void MythForest::OnSize(const Int2& size) {
	eventListenerComponentModule.OnSize(size);
}

void MythForest::OnMouse(const IFrame::EventMouse& mouse) {
	eventListenerComponentModule.OnMouse(mouse);
}

void MythForest::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
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
