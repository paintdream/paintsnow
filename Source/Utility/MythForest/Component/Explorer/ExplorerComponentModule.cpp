#include "ExplorerComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ExplorerComponentModule::ExplorerComponentModule(Engine& engine) : BaseClass(engine) {}
ExplorerComponentModule::~ExplorerComponentModule() {}

TObject<IReflect>& ExplorerComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetProxyConfig)[ScriptMethod = "SetProxyConfig"];
	}

	return *this;
}

TShared<ExplorerComponent> ExplorerComponentModule::RequestNew(IScript::Request& request, const String& componentType) {
	CHECK_REFERENCES_NONE();

	// Convert componentType from string
	unordered_map<String, Module*>::const_iterator it = engine.GetModuleMap().find(componentType);
	if (it != engine.GetModuleMap().end()) {
		request.Error(String("Unable to load component type: ") + componentType);
		return nullptr;
	} else {
		TShared<ExplorerComponent> explorerComponent = TShared<ExplorerComponent>::From(allocator->New((*it).second->GetTinyUnique()));
		explorerComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
		return explorerComponent;
	}
}

void ExplorerComponentModule::RequestSetProxyConfig(IScript::Request& request, IScript::Delegate<ExplorerComponent> explorerComponent, IScript::Delegate<Component> component, uint32_t layer, float activateThreshold, float deactivateThreshold) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(explorerComponent);
	CHECK_DELEGATE(component);

	ExplorerComponent::ProxyConfig config;
	config.layer = layer;
	config.activateThreshold = activateThreshold;
	config.deactivateThreshold = deactivateThreshold;

	explorerComponent->SetProxyConfig(component.Get(), config);
}
