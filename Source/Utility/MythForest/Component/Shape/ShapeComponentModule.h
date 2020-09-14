// ShapeComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "ShapeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class ShapeComponent;
	class ShapeComponentModule : public TReflected<ShapeComponentModule, ModuleImpl<ShapeComponent> > {
	public:
		ShapeComponentModule(Engine& engine);
		~ShapeComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ShapeComponent> RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> meshResource);
		void RequestRebuild(IScript::Request& request, IScript::Delegate<ShapeComponent> shapeComponent, Float4& color);
	};
}

