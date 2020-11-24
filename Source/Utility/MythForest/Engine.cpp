#include "Engine.h"
#include "Module.h"
#include "Entity.h"
#include "../../General/Interface/IRender.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "../SnowyStream/SnowyStream.h"

using namespace PaintsNow;

Engine::Engine(Interfaces& pinterfaces, BridgeSunset& pbridgeSunset, SnowyStream& psnowyStream) : ISyncObject(pinterfaces.thread), interfaces(pinterfaces), bridgeSunset(pbridgeSunset), snowyStream(psnowyStream) {
	unitCount.store(0, std::memory_order_relaxed);
	finalizeEvent = interfaces.thread.NewEvent();
	frameTasks.resize(GetKernel().GetWarpCount());

	IRender::Device* device = snowyStream.GetRenderDevice();
	IRender& render = interfaces.render;
	warpResourceQueues.resize(GetKernel().GetWarpCount(), nullptr);

	for (size_t i = 0; i < warpResourceQueues.size(); i++) {
		warpResourceQueues[i] = render.CreateQueue(device);
	}
}

void Engine::QueueFrameRoutine(ITask* task) {
	uint32_t warp = GetKernel().GetCurrentWarpIndex();
	assert(warp != ~(uint32_t)0);

	frameTasks[warp].push(task);
}

IRender::Queue* Engine::GetWarpResourceQueue() {
	return warpResourceQueues[GetKernel().GetCurrentWarpIndex()];
}

Engine::~Engine() {
	assert(modules.empty());
	interfaces.thread.DeleteEvent(finalizeEvent);
}

void Engine::Clear() {
	for (std::unordered_map<String, Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
		(*it).second->Uninitialize();
	}

	for (size_t i = 0; i < frameTasks.size(); i++) {
		std::queue<ITask*>& q = frameTasks[i];
		while (!q.empty()) {
			ITask* task = q.front();
			q.pop();

			task->Abort(this);
		}
	}

	while (unitCount.load(std::memory_order_acquire) != 0) {
		DoLock();
		threadApi.Wait(finalizeEvent, mutex, 50);
		UnLock();
	}

	for (std::unordered_map<String, Module*>::iterator ip = modules.begin(); ip != modules.end(); ++ip) {
		(*ip).second->ReleaseObject();
	}

	modules.clear();

	IRender& render = interfaces.render;
	for (size_t j = 0; j < warpResourceQueues.size(); j++) {
		render.DeleteQueue(warpResourceQueues[j]);
	}

	warpResourceQueues.clear();
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
	IRender& render = interfaces.render;
	IRender::Queue* resourceQueue = snowyStream.GetResourceQueue();
	if (resourceQueue != nullptr) {
		interfaces.render.PresentQueues(&resourceQueue, 1, IRender::PRESENT_EXECUTE_ALL);
	}

	for (size_t i = 0; i < warpResourceQueues.size(); i++) {
		render.PresentQueues(&warpResourceQueues[i], 1, IRender::PRESENT_EXECUTE_ALL);
	}

	for (size_t j = 0; j < frameTasks.size(); j++) {
		std::queue<ITask*>& q = frameTasks[j];
		while (!q.empty()) {
			ITask* task = q.front();
			q.pop();

			task->Execute(this);
		}
	}

	for (std::unordered_map<String, Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
		(*it).second->TickFrame();
	}

	frameIndex.fetch_add(1, std::memory_order_relaxed);
}

uint32_t Engine::GetFrameIndex() const {
	return frameIndex.load(std::memory_order_relaxed);
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