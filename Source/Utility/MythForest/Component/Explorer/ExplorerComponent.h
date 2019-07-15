// ExplorerComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __EXPLORERCOMPONENT_H__
#define __EXPLORERCOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ExplorerComponent : public TAllocatedTiny<ExplorerComponent, Component> {
		public:
			ExplorerComponent();
			virtual ~ExplorerComponent();
		};
	}
}


#endif // __EXPLORERCOMPONENT_H__