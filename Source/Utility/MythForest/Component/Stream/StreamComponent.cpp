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

void StreamComponent::Unload(Engine& engine, const UShort3& coord, TShared<SharedTiny> context) {
	uint16_t id = idGrids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	if (id == ~(uint16_t)0) return;

	UnloadInternal(engine, grids[id], context);
}

void StreamComponent::UnloadInternal(Engine& engine, Grid& grid, TShared<SharedTiny> context) {
	if (unloadHandler.script) {
		IScript::Request& request = engine.bridgeSunset.AllocateRequest();
		IScript::Delegate<SharedTiny> w;

		request.DoLock();
		request.Push();
		request.Call(sync, unloadHandler.script, grid.coord, grid.object, context);
		request >> w;
		request.Pop();
		request.UnLock();

		grid.object = w.Get();
		engine.bridgeSunset.FreeRequest(request);
	} else if (unloadHandler.native) {
		grid.object = unloadHandler.native(engine, grid.coord, grid.object, context);
	}
}

SharedTiny* StreamComponent::Load(Engine& engine, const UShort3& coord, TShared<SharedTiny> context) {
	uint16_t id = idGrids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	SharedTiny* object = nullptr;
	if (id == ~(uint16_t)0) {
		// allocate id ...
		id = recycleQueue[recycleStart];

		Grid& grid = grids[id];
		if (grid.object) {
			UnloadInternal(engine, grid, context);
		}

		grid.recycleIndex = recycleStart;
		recycleStart = (recycleStart + 1) % safe_cast<uint16_t>(recycleQueue.size());

		if (loadHandler.script) {
			IScript::Request& request = engine.bridgeSunset.AllocateRequest();
			IScript::Delegate<SharedTiny> w;

			request.DoLock();
			request.Push();
			request.Call(sync, loadHandler.script, coord, grid.object, context);
			request >> w;
			request.Pop();
			request.UnLock();

			assert(w);
			grid.object = w.Get();
			engine.bridgeSunset.FreeRequest(request);
		} else {
			assert(loadHandler.native);
			grid.object = loadHandler.native(engine, coord, grid.object, context);
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

const UShort3& StreamComponent::GetDimension() const {
	return dimension;
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

void StreamComponent::SetLoadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> >& handler) {
	loadHandler.native = handler;
}

void StreamComponent::SetUnloadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> >& handler) {
	unloadHandler.native = handler;
}

