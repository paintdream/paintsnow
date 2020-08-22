// TerrainComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "TerrainComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class TerrainComponent;
	class TerrainComponentModule : public TReflected<TerrainComponentModule, ModuleImpl<TerrainComponent> > {
	public:
		TerrainComponentModule(Engine& engine);

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<TerrainComponent> RequestNew(IScript::Request& request, IScript::Delegate<TerrainResource> terrainResource);
		void RequestRebuild(IScript::Request& request, IScript::Delegate<TerrainComponent> terrainComponent);
	};
}
