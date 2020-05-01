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
	assert(grid.object);

	TShared<SharedTiny> object = std::move(grid.object);
	grid.object = nullptr;

	if (unloadHandler.script) {
		IScript::Request& request = engine.bridgeSunset.AllocateRequest();

		request.DoLock();
		request.Push();
		request.Call(sync, unloadHandler.script, grid.coord, object);
		request.Pop();
		request.UnLock();

		engine.bridgeSunset.FreeRequest(request);
	} else if (unloadHandler.native) {
		unloadHandler.native(engine, grid.coord, object);
	}
}

SharedTiny* StreamComponent::Load(Engine& engine, const UShort3& coord) {
	uint16_t id = idGrids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	SharedTiny* object = nullptr;
	if (id == ~(uint16_t)0) {
		// allocate id ...
		id = recycleQueue[recycleStart];

		Grid& grid = grids[id];
		if (grid.object) {
			UnloadInternal(engine, grid);
			assert(!grid.object);
		}

		grid.recycleIndex = recycleStart;
		recycleStart = (recycleStart + 1) % safe_cast<uint16_t>(recycleQueue.size());

		if (loadHandler.script) {
			IScript::Request& request = engine.bridgeSunset.AllocateRequest();
			IScript::Delegate<Component> w;

			request.DoLock();
			request.Push();
			request.Call(sync, loadHandler.script, coord);
			request >> w;
			request.Pop();
			request.UnLock();

			assert(w);
			grid.object = w.Get();
			engine.bridgeSunset.FreeRequest(request);
		} else {
			assert(loadHandler.native);
			grid.object = loadHandler.native(engine, coord);
		}

		grid.coord = coord;
	} else {
		Grid& grid = grids[id];
		uint16_t oldIndex = grid.recycleIndex;
		grid.recycleIndex = recycleStart;
		recycleQueue[oldIndex] = recycleQueue[recycleStart]; // make swap
		recycleQueue[recycleStart] = id;

		recycleStart = (recycleStart + 1) % safe_cast<uint16_t>(recycleQueue.size());
	}

	return object;
}

void StreamComponent::Uninitialize(Engine& engine, Entity* entity) {
	IScript::Request& request = engine.interfaces.script.GetDefaultRequest();
	SetLoadHandler(request, IScript::Request::Ref());
	SetUnloadHandler(request, IScript::Request::Ref());

	Component::Uninitialize(engine, entity);
}


void StreamComponent::SetLoadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	loadHandler.ReplaceScript(request, ref);
}

void StreamComponent::SetUnloadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	unloadHandler.ReplaceScript(request, ref);
}

void StreamComponent::SetLoadHandler(IScript::Request& request, const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&>& handler) {
	loadHandler.native = handler;
}

void StreamComponent::SetUnloadHandler(IScript::Request& request, const TWrapper<void, Engine&, const UShort3&, TShared<SharedTiny> >& handler) {
	unloadHandler.native = handler;
}

