// RasterizeComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "RasterizeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class RasterizeComponent;
	class RasterizeComponentModule : public TReflected<RasterizeComponentModule, ModuleImpl<RasterizeComponent> > {
	public:
		RasterizeComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create RasterizeComponent
		/// </summary>
		/// <returns> RasterizeComponent object </returns>
		TShared<RasterizeComponent> RequestNew(IScript::Request& request);
	};
}
