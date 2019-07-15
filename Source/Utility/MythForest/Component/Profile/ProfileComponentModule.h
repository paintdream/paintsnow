// ProfileComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __PROFILECOMPONENTMODULE_H__
#define __PROFILECOMPONENTMODULE_H__

#include "ProfileComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ProfileComponent;
		class ProfileComponentModule  : public TReflected<ProfileComponentModule , ModuleImpl<ProfileComponent> > {
		public:
			ProfileComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request, float historyRatio);
			void RequestGetInterval(IScript::Request& request, IScript::Delegate<ProfileComponent> profileComponent);
		};
	}
}


#endif // __PROFILECOMPONENTMODULE_H__
