#include "StreamComponent.h"

#include <utility>
#include "../../../BridgeSunset/BridgeSunset.h"

using namespace PaintsNow;

StreamComponent::StreamComponent(const UShort3& dim, uint16_t cacheCount) : dimension(dim), recycleStart(0) {
	assert(dim.x() != 0 && dim.y() != 0 && dim.z() != 0);
	idGrids.resize(dim.x() * dim.y() * dim.z(), (uint16_t)~0);
	grids.resize(cacheCount);
	recycleQueue.resize(cacheCount);
	for (uint16_t i = 0; i < cacheCount; i++) {
		recycleQueue[i] = i;
	}
}

UShort3 StreamComponent::ComputeWrapCoordinate(const Int3& pos) const {
	return UShort3(
		safe_cast<uint16_t>((pos.x() % dimension.x() + dimension.x()) % dimension.x()),
		safe_cast<uint16_t>((pos.y() % dimension.y() + dimension.y()) % dimension.y()),
		safe_cast<uint16_t>((pos.z() % dimension.z() + dimension.z()) % dimension.z()));
}

uint16_t StreamComponent::GetCacheCount() const {
	return safe_cast<uint16_t>(grids.size());
}

void StreamComponent::Unload(Engine& engine, const UShort3& coord, const TShared<SharedTiny>&context) {
	uint16_t id = idGrids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	if (id == (uint16_t)~0) return;

	UnloadInternal(engine, grids[id], context);
}

void StreamComponent::UnloadInternal(Engine& engine, Grid& grid, const TShared<SharedTiny>&context) {
	if (unloadHandler.script) {
		IScript::Request& request = *engine.bridgeSunset.requestPool.AcquireSafe();
		IScript::Delegate<SharedTiny> w;

		request.DoLock();
		request.Push();
		request.Call(unloadHandler.script, grid.coord, grid.object, context);
		request >> w;
		request.Pop();
		request.UnLock();

		grid.object = w.Get();
		engine.bridgeSunset.requestPool.ReleaseSafe(&request);
	} else if (unloadHandler.native) {
		grid.object = unloadHandler.native(engine, grid.coord, grid.object, context);
	}
}

SharedTiny* StreamComponent::Load(Engine& engine, const UShort3& coord, const TShared<SharedTiny>&context) {
	size_t offset = (coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x();
	uint16_t id = idGrids[offset];
	SharedTiny* object = nullptr;
	if (id == (uint16_t)~0) {
		// allocate id ...
		id = recycleQueue[recycleStart];

		Grid& grid = grids[id];
		TShared<SharedTiny> last = grid.object;

		if (grid.object) {
			UnloadInternal(engine, grid, context);
		}

		grid.recycleIndex = recycleStart;
		recycleStart = (recycleStart + 1) % safe_cast<uint16_t>(recycleQueue.size());

		if (loadHandler.script) {
			IScript::Request& request = *engine.bridgeSunset.requestPool.AcquireSafe();
			IScript::Delegate<SharedTiny> w;

			request.DoLock();
			request.Push();
			request.Call(loadHandler.script, coord, last, context);
			request >> w;
			request.Pop();
			request.UnLock();

			assert(w);
			grid.object = w.Get();
			engine.bridgeSunset.requestPool.ReleaseSafe(&request);
		} else {
			assert(loadHandler.native);
			grid.object = loadHandler.native(engine, coord, last, context);
		}

		grid.coord = coord;
		object = grid.object();
		idGrids[offset] = id;
	} else {
		Grid& grid = grids[id];
		uint16_t oldIndex = grid.recycleIndex;
		grid.recycleIndex = recycleStart;
		recycleQueue[oldIndex] = recycleQueue[recycleStart]; // make swap
		recycleQueue[recycleStart] = id;

		recycleStart = (recycleStart + 1) % safe_cast<uint16_t>(recycleQueue.size());
		object = grid.object();
	}

	return object;
}

const UShort3& StreamComponent::GetDimension() const {
	return dimension;
}

void StreamComponent::Uninitialize(Engine& engine, Entity* entity) {
	if (!engine.interfaces.script.IsClosing()) {
		IScript::Request& request = engine.interfaces.script.GetDefaultRequest();
		SetLoadHandler(request, IScript::Request::Ref());
		SetUnloadHandler(request, IScript::Request::Ref());
	} else {
		loadHandler.script = IScript::Request::Ref();
		unloadHandler.script = IScript::Request::Ref();
	}

	Component::Uninitialize(engine, entity);
}

void StreamComponent::SetLoadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	loadHandler.ReplaceScript(request, ref);
}

void StreamComponent::SetUnloadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	unloadHandler.ReplaceScript(request, ref);
}

void StreamComponent::SetLoadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& >& handler) {
	loadHandler.native = handler;
}

void StreamComponent::SetUnloadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>& >& handler) {
	unloadHandler.native = handler;
}

