#include "Engine.h"
#include "Module.h"
#include "Entity.h"
#include "../../General/Interface/IRender.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "../SnowyStream/SnowyStream.h"
#include "../SnowyStream/Manager/RenderResourceManager.h"

using namespace PaintsNow;

Engine::Engine(Interfaces& pinterfaces, BridgeSunset& pbridgeSunset, SnowyStream& psnowyStream) : ISyncObject(pinterfaces.thread), interfaces(pinterfaces), bridgeSunset(pbridgeSunset), snowyStream(psnowyStream) {
	unitCount.store(0, std::memory_order_relaxed);
	finalizeEvent = interfaces.thread.NewEvent();
	
}

Engine::~Engine() {
	assert(modules.empty());
	interfaces.thread.DeleteEvent(finalizeEvent);
}

void Engine::Clear() {
	for (std::unordered_map<String, Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
		(*it).second->Uninitialize();
	}

	while (unitCount.load(std::memory_order_acquire) != 0) {
		DoLock();
		threadApi.Wait(finalizeEvent, mutex, 50);
		UnLock();
	}

	for (std::unordered_map<String, Module*>::iterator ip = modules.begin(); ip != modules.end(); ++ip) {
		(*ip).second->Destroy();
	}

	modules.clear();
}

void Engine::InstallModule(Module* module) {
	modules[module->GetTinyUnique()->GetBriefName()] = module;
	module->Initialize();
}

Module* Engine::GetComponentModuleFromName(const String& name) const {
	std::unordered_map<String, Module*>::const_iterator it = modules.find(name);
	if (it != modules.end()) {
		return (*it).second;
	} else {
		return nullptr;
	}
}

std::unordered_map<String, Module*>& Engine::GetModuleMap() {
	return modules;
}

void Engine::TickFrame() {
	for (std::unordered_map<String, Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
		(*it).second->TickFrame();
	}
}

Kernel& Engine::GetKernel() {
	return bridgeSunset.GetKernel();
}

void Engine::NotifyUnitConstruct(Unit* unit) {
	unitCount.fetch_add(1, std::memory_order_relaxed);
#if defined(_DEBUG)
	singleton Unique entityUnique = UniqueType<Entity>::Get();
	if (unit->GetUnique() != entityUnique) return;

	SpinLock(unitCritical);
	assert(entityMap.find(unit) == entityMap.end());
	entityMap[unit] = nullptr;
	SpinUnLock(unitCritical);
#endif
}

void Engine::NotifyUnitDestruct(Unit* unit) {
	if (unitCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
		interfaces.thread.Signal(finalizeEvent);
	}

#if defined(_DEBUG)
	singleton Unique entityUnique = UniqueType<Entity>::Get();
	if (unit->GetUnique() != entityUnique) return;

	SpinLock(unitCritical);
	assert(entityMap.find(unit) != entityMap.end());
	entityMap.erase(unit);
	SpinUnLock(unitCritical);
#endif
}

void Engine::NotifyUnitAttach(Unit* unit, Unit* parent) {
#if defined(_DEBUG)
	singleton Unique entityUnique = UniqueType<Entity>::Get();
	assert(unit->GetUnique() == entityUnique && parent->GetUnique() == entityUnique);

	SpinLock(unitCritical);
	assert(parent != nullptr);
	assert(entityMap.find(unit) != entityMap.end());
	assert(entityMap[unit] == nullptr);
	Unit* p = parent;
	while (true) {
		assert(p != unit); // cycle detected
		std::map<Unit*, Unit*>::const_iterator it = entityMap.find(p);
		if (it == entityMap.end())
			break;

		p = (*it).second;
	}

	entityMap[unit] = parent;
	SpinUnLock(unitCritical);
#endif
}

void Engine::NotifyUnitDetach(Unit* unit) {
#if defined(_DEBUG)
	singleton Unique entityUnique = UniqueType<Entity>::Get();
	assert(unit->GetUnique() == entityUnique);

	SpinLock(unitCritical);
	assert(entityMap.find(unit) != entityMap.end());
	assert(entityMap[unit] != nullptr);
	entityMap[unit] = nullptr;
	SpinUnLock(unitCritical);
#endif
}