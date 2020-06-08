// SurfaceComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __SURFACECOMPONENTMODULE_H__
#define __SURFACECOMPONENTMODULE_H__

#include "SurfaceComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class SurfaceComponent;
		class SurfaceComponentModule  : public TReflected<SurfaceComponentModule, ModuleImpl<SurfaceComponent> > {
		public:
			SurfaceComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<SurfaceComponent> RequestNew(IScript::Request& request);
			void RequestRebuild(IScript::Request& request, IScript::Delegate<SurfaceComponent> surfaceComponent);
		};
	}
}


#endif // __SURFACECOMPONENTMODULE_H__
