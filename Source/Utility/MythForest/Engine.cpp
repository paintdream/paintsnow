#include "Engine.h"
#include "Module.h"
#include "Entity.h"
#include "../../General/Interface/IRender.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "../SnowyStream/SnowyStream.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsBridgeSunset;

Engine::Engine(Interfaces& pinterfaces, NsBridgeSunset::BridgeSunset& pbridgeSunset, NsSnowyStream::SnowyStream& psnowyStream) : ISyncObject(pinterfaces.thread), interfaces(pinterfaces), bridgeSunset(pbridgeSunset), snowyStream(psnowyStream) {
	entityCount.store(0, std::memory_order_relaxed);
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

	frameTasks[warp].Push(task);
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
		TQueue<ITask*>& q = frameTasks[i];
		while (!q.Empty()) {
			ITask* task = q.Top();
			q.Pop();

			task->Abort(this);
		}
	}

	while (entityCount.load(std::memory_order_acquire) != 0) {
		threadApi.Wait(finalizeEvent, mutex, 50);
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
	for (size_t i = 0; i < warpResourceQueues.size(); i++) {
		render.PresentQueues(&warpResourceQueues[i], 1, IRender::PRESENT_EXECUTE_ALL);
	}

	for (size_t j = 0; j < frameTasks.size(); j++) {
		TQueue<ITask*>& q = frameTasks[j];
		while (!q.Empty()) {
			ITask* task = q.Top();
			q.Pop();

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

void Engine::NotifyEntityConstruct(Entity* entity) {
	entityCount.fetch_add(1, std::memory_order_release);
#ifdef _DEBUG
	SpinLock(entityCritical);
	assert(entityMap.find(entity) == entityMap.end());
	entityMap[entity] = nullptr;
	SpinUnLock(entityCritical);
#endif
}

void Engine::NotifyEntityDestruct(Entity* entity) {
	if (entityCount.fetch_sub(1, std::memory_order_release) == 1) {
		interfaces.thread.Signal(finalizeEvent, false);
	}

#ifdef _DEBUG
	SpinLock(entityCritical);
	assert(entityMap.find(entity) != entityMap.end());
	entityMap.erase(entity);
	SpinUnLock(entityCritical);
#endif
}

void Engine::NotifyEntityAttach(Entity* entity, Entity* parent) {
#ifdef _DEBUG
	SpinLock(entityCritical);
	assert(parent != nullptr);
	assert(entityMap.find(entity) != entityMap.end());
	assert(entityMap[entity] == nullptr);
	Entity* p = parent;
	while (true) {
		assert(p != entity); // cycle detected
		std::unordered_map<Entity*, Entity*>::iterator it = entityMap.find(p);
		if (it == entityMap.end())
			break;

		p = (*it).second;
	}

	entityMap[entity] = parent;
	SpinUnLock(entityCritical);
#endif
}

void Engine::NotifyEntityDetach(Entity* entity) {
#ifdef _DEBUG
	SpinLock(entityCritical);
	assert(entityMap.find(entity) != entityMap.end());
	assert(entityMap[entity] != nullptr);
	entityMap[entity] = nullptr;
	SpinUnLock(entityCritical);
#endif
}