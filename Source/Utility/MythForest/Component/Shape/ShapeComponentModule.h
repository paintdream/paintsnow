// ShapeComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __SHAPECOMPONENTMODULE_H__
#define __SHAPECOMPONENTMODULE_H__

#include "ShapeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class ShapeComponent;
		class ShapeComponentModule  : public TReflected<ShapeComponentModule , ModuleImpl<ShapeComponent> > {
		public:
			ShapeComponentModule(Engine& engine);
			virtual ~ShapeComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request, IScript::Delegate<NsSnowyStream::MeshResource> meshResource);
			void RequestRebuild(IScript::Request& request, IScript::Delegate<ShapeComponent> shapeComponent, Float4& color);
		};
	}
}


#endif // __SHAPECOMPONENTMODULE_H__