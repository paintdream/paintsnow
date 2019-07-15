#include "Engine.h"
#include "Module.h"
#include "Entity.h"
#include "../../General/Interface/IRender.h"
#include "../BridgeSunset/BridgeSunset.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsBridgeSunset;

Engine::Engine(Interfaces& pinterfaces, NsBridgeSunset::BridgeSunset& pbridgeSunset, NsSnowyStream::SnowyStream& psnowyStream, NsMythForest::MythForest& pmythForest) : ISyncObject(pinterfaces.thread), interfaces(pinterfaces), bridgeSunset(pbridgeSunset), snowyStream(psnowyStream), mythForest(pmythForest) {
	entityCount.store(0, std::memory_order_relaxed);
	finalizeEvent = interfaces.thread.NewEvent();
}

Engine::~Engine() {
	assert(modules.empty());
	interfaces.thread.DeleteEvent(finalizeEvent);
}

void Engine::Clear() {
	for (unordered_map<String, Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
		(*it).second->Uninitialize();
	}

	while (entityCount.load(std::memory_order_acquire) != 0) {
		threadApi.Wait(finalizeEvent, mutex, 50);
	}

	modules.clear();
}

void Engine::InstallModule(Module* module) {
	modules[module->GetTinyUnique()->GetSubName()] = module;
	module->Initialize();
}

void Engine::UninstallModule(Module* module) {
	unordered_map<String, Module*>::iterator it = modules.find(module->GetTinyUnique()->GetSubName());
	if (it != modules.end()) {
		(*it).second->Uninitialize();
		modules.erase(it);
	}
}

Module* Engine::GetComponentModuleFromName(const String& name) const {
	unordered_map<String, Module*>::const_iterator it = modules.find(name);
	if (it != modules.end()) {
		return (*it).second;
	} else {
		return nullptr;
	}
}

const unordered_map<String, Module*>& Engine::GetModuleMap() const {
	return modules;
}

void Engine::TickFrame() {
	for (unordered_map<String, Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
		(*it).second->TickFrame();
	}
}

Kernel& Engine::GetKernel() {
	return bridgeSunset.GetKernel();
}