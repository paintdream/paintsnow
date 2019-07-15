// NavigateComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __NAVIGATECOMPONENT_H__
#define __NAVIGATECOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	namespace NsMythForest {
		class NavigateComponent : public TAllocatedTiny<NavigateComponent, Component> {
		public:
			NavigateComponent();
			virtual ~NavigateComponent();
		};
	}
}


#endif // __NAVIGATECOMPONENT_H__