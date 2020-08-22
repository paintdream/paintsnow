#include "TerrainComponentModule.h"

using namespace PaintsNow;

TerrainComponentModule::TerrainComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& TerrainComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestRebuild)[ScriptMethod = "Rebuild"];
	}

	return *this;
}

TShared<TerrainComponent> TerrainComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<TerrainResource> terrainResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(terrainResource);

	TShared<TerrainComponent> terrainComponent = TShared<TerrainComponent>::From(allocator->New());
	terrainComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return terrainComponent;
}

void TerrainComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<TerrainComponent> terrainComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(terrainComponent);


}