// RayTraceComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "RayTraceComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class RayTraceComponent;
	class RayTraceComponentModule : public TReflected<RayTraceComponentModule, ModuleImpl<RayTraceComponent> > {
	public:
		RayTraceComponentModule(Engine& engine);
		~RayTraceComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create RayTraceComponent
		/// </summary>
		/// <returns> RayTraceComponent object </returns>
		TShared<RayTraceComponent> RequestNew(IScript::Request& request);
	};
}

