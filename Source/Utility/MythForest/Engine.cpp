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
	frameTasks.resize(GetKernel().GetWarpCount());
}

void Engine::QueueFrameRoutine(ITask* task) {
	uint32_t warp = GetKernel().GetCurrentWarpIndex();
	assert(warp != ~(uint32_t)0);

	frameTasks[warp].Push(task);
}

Engine::~Engine() {
	assert(modules.empty());
	interfaces.thread.DeleteEvent(finalizeEvent);
}

void Engine::Clear() {
	for (unordered_map<String, Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
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

	for (unordered_map<String, Module*>::iterator ip = modules.begin(); ip != modules.end(); ++ip) {
		(*ip).second->ReleaseObject();
	}

	modules.clear();
}

void Engine::InstallModule(Module* module) {
	modules[module->GetTinyUnique()->GetSubName()] = module;
	module->Initialize();
}

Module* Engine::GetComponentModuleFromName(const String& name) const {
	unordered_map<String, Module*>::const_iterator it = modules.find(name);
	if (it != modules.end()) {
		return (*it).second;
	} else {
		return nullptr;
	}
}

unordered_map<String, Module*>& Engine::GetModuleMap() {
	return modules;
}

void Engine::TickFrame() {
	for (size_t i = 0; i < frameTasks.size(); i++) {
		TQueue<ITask*>& q = frameTasks[i];
		while (!q.Empty()) {
			ITask* task = q.Top();
			q.Pop();

			task->Execute(this);
		}
	}

	for (unordered_map<String, Module*>::iterator it = modules.begin(); it != modules.end(); ++it) {
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