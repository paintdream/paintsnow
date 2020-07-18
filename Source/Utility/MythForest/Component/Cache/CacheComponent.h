// CacheComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#ifndef __CACHECOMPONENT_H__
#define __CACHECOMPONENT_H__

#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/Interface/IType.h"

namespace PaintsNow {
	namespace NsMythForest {
		class CacheComponent : public TAllocatedTiny<CacheComponent, Component> {
		public:
			CacheComponent();
			void PushObjects(rvalue<std::vector<TShared<SharedTiny> > > objects);
			void ClearObjects();

		protected:
			std::vector<TShared<SharedTiny> > cachedObjects;
		};
	}
}


#endif // __CACHECOMPONENT_H__