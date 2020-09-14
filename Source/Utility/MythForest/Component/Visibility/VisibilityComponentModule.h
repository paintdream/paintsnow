// VisibilityComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "VisibilityComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class VisibilityComponent;
	class VisibilityComponentModule : public TReflected<VisibilityComponentModule, ModuleImpl<VisibilityComponent> > {
	public:
		VisibilityComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<VisibilityComponent> RequestNew(IScript::Request& request);
		void RequestSetup(IScript::Request& request, IScript::Delegate<VisibilityComponent> visibilityComponent, float maxDistance, const Float3Pair& range, const UShort3& division, uint32_t frameTimeLimit, uint32_t taskCount, const UShort2& resolution);
	};
}
