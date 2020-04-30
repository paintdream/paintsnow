#include "StreamComponent.h"
#include "../../../BridgeSunset/BridgeSunset.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

StreamComponent::StreamComponent(const UShort3& dim, uint32_t cacheCount) : dimension(dim) {
	idGrids.resize(dim.x() * dim.y() * dim.z());
	grids.resize(cacheCount);
}

void StreamComponent::Unload(Engine& engine, const UShort3& coord) {
	Grid& grid = grids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	if (grid.component) {
		TShared<Component> component = grid.component;
		grid.component = nullptr;

		if (unloadHandler) {
			IScript::Request& request = engine.bridgeSunset.AllocateRequest();

			request.DoLock();
			request.Push();
			request.Call(sync, unloadHandler, coord, component);
			request.Pop();
			request.UnLock();

			engine.bridgeSunset.FreeRequest(request);
		}
	}
}

Component* StreamComponent::Load(Engine& engine, const UShort3& coord) {
	Grid& grid = grids[(coord.z() * dimension.y() + coord.y()) * dimension.x() + coord.x()];
	if (!grid.component) {
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
	}

	return grid.component();
}

void StreamComponent::Uninitialize(Engine& engine, Entity* entity) {
	IScript::Request& request = engine.interfaces.script.GetDefaultRequest();
	SetLoadHandler(request, IScript::Request::Ref());
	SetUnloadHandler(request, IScript::Request::Ref());

	Component::Uninitialize(engine, entity);
}

void StreamComponent::SetLoadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	ReplaceHandler(request, loadHandler, ref);
}

void StreamComponent::SetUnloadHandler(IScript::Request& request, IScript::Request::Ref ref) {
	ReplaceHandler(request, unloadHandler, ref);
}

void StreamComponent::ReplaceHandler(IScript::Request& request, IScript::Request::Ref& target, IScript::Request::Ref ref) {
	if (target != ref) {
		request.DoLock();
		request.Dereference(target);
		target = ref;
		request.UnLock();
	}
}
