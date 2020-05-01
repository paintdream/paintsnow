#include "StreamComponent.h"
#include "../../../BridgeSunset/BridgeSunset.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

StreamComponent::StreamComponent(const UShort3& dim, uint16_t cacheCount) : dimension(dim), recycleStart(0) {
	idGrids.resize(dim.x() * dim.y() * dim.z());
	grids.resize(cacheCount);
	recycleQueue.resize(cacheCount);
	for (uint16_t i = 0; i < cacheCount; i++) {
		recycleQueue[i] = i;
	}
}

void StreamComponent::Unload(Engine& engine, const UShort3& coord) {
	uint16_t id = idGrids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	if (id == ~(uint16_t)0) return;

	UnloadInternal(engine, grids[id]);
}

void StreamComponent::UnloadInternal(Engine& engine, Grid& grid) {
	assert(grid.component);

	TShared<Component> component = grid.component;
	grid.component = nullptr;

	if (unloadHandler) {
		IScript::Request& request = engine.bridgeSunset.AllocateRequest();

		request.DoLock();
		request.Push();
		request.Call(sync, unloadHandler, grid.coord, component);
		request.Pop();
		request.UnLock();

		engine.bridgeSunset.FreeRequest(request);
	}
}

Component* StreamComponent::Load(Engine& engine, const UShort3& coord) {
	uint16_t id = idGrids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	Component* component = nullptr;
	if (id == ~(uint16_t)0) {
		// allocate id ...
		id = recycleQueue[recycleStart];

		Grid& grid = grids[id];
		if (grid.component) {
			UnloadInternal(engine, grid);
			assert(!grid.component);
		}

		grid.recycleIndex = recycleStart;
		recycleStart = (recycleStart + 1) % safe_cast<uint16_t>(recycleQueue.size());

		IScript::Request& request = engine.bridgeSunset.AllocateRequest();
		IScript::Delegate<Component> w;

		request.DoLock();
		request.Push();
		request.Call(sync, loadHandler, coord);
		request >> w;
		request.Pop();
		request.UnLock();

		assert(w);
		grid.component = w.Get();
		grid.coord = coord;

		engine.bridgeSunset.FreeRequest(request);
	} else {
		Grid& grid = grids[id];
		uint16_t oldIndex = grid.recycleIndex;
		grid.recycleIndex = recycleStart;
		recycleQueue[oldIndex] = recycleQueue[recycleStart]; // make swap
		recycleQueue[recycleStart] = id;

		recycleStart = (recycleStart + 1) % safe_cast<uint16_t>(recycleQueue.size());
	}

	return component;
}

void StreamComponent::Uninitialize(Engine& engine, Entity* entity) {
	IScript::Request& request = engine.interfaces.script.GetDefaultRequest();
	SetLoadHandler(request, IScript::Request::Ref());
	SetUnloadHandler(request, IScript::Request::Ref());

	Component::Uninitialize(engine, entity);
}

static void ReplaceHandler(IScript::Request& request, IScript::Request::Ref& target, IScript::Request::Ref ref) {
	if (target != ref) {
		request.DoLock();
		request.Dereference(target);
		target = ref;
		request.UnLock();
	}
}

void StreamComponent::SetLoadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	ReplaceHandler(request, loadHandler, ref);
}

void StreamComponent::SetUnloadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	ReplaceHandler(request, unloadHandler, ref);
}

