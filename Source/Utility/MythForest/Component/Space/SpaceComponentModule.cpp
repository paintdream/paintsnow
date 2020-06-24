#include "SpaceComponentModule.h"
#include "../../Entity.h"
#include "../../../BridgeSunset/BridgeSunset.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::NsBridgeSunset;

// Script interfaces
SpaceComponentModule::SpaceComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& SpaceComponentModule::operator ()(IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestInsertEntity)[ScriptMethod = "InsertEntity"];
		ReflectMethod(RequestRemoveEntity)[ScriptMethod = "RemoveEntity"];
		ReflectMethod(RequestQueryEntities)[ScriptMethod = "QueryEntities"];
		ReflectMethod(RequestSetForwardMask)[ScriptMethod = "SetForwardMask"];
		ReflectMethod(RequestGetEntityCount)[ScriptMethod = "GetEntityCount"];
	}

	return *this;
}

TShared<SpaceComponent> SpaceComponentModule::RequestNew(IScript::Request& request, int32_t warpIndex, bool sorted) {
	TShared<SpaceComponent> spaceComponent = TShared<SpaceComponent>::From(allocator->New(warpIndex, sorted));
	spaceComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return spaceComponent;
}

void SpaceComponentModule::RequestSetForwardMask(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, uint32_t forwardMask) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);

	if (forwardMask != 0) {
		spaceComponent->Flag().fetch_or(SpaceComponent::SPACECOMPONENT_FORWARD_EVENT_TICK, std::memory_order_acquire);
	} else {
		spaceComponent->Flag().fetch_and(~SpaceComponent::SPACECOMPONENT_FORWARD_EVENT_TICK, std::memory_order_release);
	}
}

uint32_t SpaceComponentModule::RequestGetEntityCount(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);
	return spaceComponent->GetEntityCount();
}


void SpaceComponentModule::RequestInsertEntity(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	if (spaceComponent->Flag() & SpaceComponent::SPACECOMPONENT_ATTACHED) {
		spaceComponent->Insert(engine, entity.Get());
	} else {
		request.Error("Isolated SpaceComponent cannot hold entities.");
	}
}

void SpaceComponentModule::RequestRemoveEntity(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	spaceComponent->Remove(engine, entity.Get());
}


void SpaceComponentModule::RequestQueryEntities(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, const Float3Pair& box) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(spaceComponent);
	CHECK_THREAD_IN_MODULE(spaceComponent);

	std::vector<TShared<Entity> > entityList;
	spaceComponent->QueryEntities(entityList, box);
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	for (size_t i = 0; i < entityList.size(); i++) {
		request << entityList[i];
	}
	request.UnLock();
}

void SpaceComponentModule::ScriptUninitialize(IScript::Request& request) {
	Module::ScriptUninitialize(request);
}
