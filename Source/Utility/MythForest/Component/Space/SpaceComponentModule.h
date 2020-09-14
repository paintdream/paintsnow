// SpaceComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-10
//

#pragma once
#include "SpaceComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class SpaceComponent;
	class SpaceComponentModule : public TReflected<SpaceComponentModule, ModuleImpl<SpaceComponent> > {
	public:
		SpaceComponentModule(Engine& engine);
		TShared<SpaceComponent> RequestNew(IScript::Request& request, int32_t warpIndex, bool sorted);
		void RequestSetForwardMask(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, uint32_t forwardMask);
		void RequestInsertEntity(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, IScript::Delegate<Entity> entity);
		void RequestRemoveEntity(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, IScript::Delegate<Entity> entity);
		void RequestQueryEntities(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, const Float3Pair& box);
		uint32_t RequestGetEntityCount(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void ScriptUninitialize(IScript::Request& request) override;
	};
}

