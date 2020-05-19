// CacheComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#ifndef __CACHECOMPONENTMODULE_H__
#define __CACHECOMPOENNTMODULE_H__

#include "CacheComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class CacheComponent;
		class CacheComponentModule  : public TReflected<CacheComponentModule, ModuleImpl<CacheComponent> > {
		public:
			CacheComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestPushObjects(IScript::Request& request, IScript::Delegate<CacheComponent> cacheComponent, std::vector<IScript::Delegate<SharedTiny> >& objects);
			void RequestClearObjects(IScript::Request& request, IScript::Delegate<CacheComponent> cacheComponent);
		};
	}
}


#endif // __CACHECOMPONENTMODULE_H__