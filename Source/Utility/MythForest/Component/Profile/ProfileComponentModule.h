// ProfileComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "ProfileComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class ProfileComponent;
	class ProfileComponentModule : public TReflected<ProfileComponentModule, ModuleImpl<ProfileComponent> > {
	public:
		ProfileComponentModule(Engine& engine);
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ProfileComponent> RequestNew(IScript::Request& request, float historyRatio);
		float RequestGetInterval(IScript::Request& request, IScript::Delegate<ProfileComponent> profileComponent);
	};
}
