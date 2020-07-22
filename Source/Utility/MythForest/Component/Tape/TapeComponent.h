// TapeComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#ifndef __TAPECOMPONENT_H__
#define __TAPECOMPONENT_H__

#include "../../Entity.h"
#include "../../Component.h"

namespace PaintsNow {
	namespace NsMythForest {
		class TapeComponent : public TAllocatedTiny<TapeComponent, Component> {
		public:
			TapeComponent();
		};
	}
}


#endif // __TAPECOMPONENT_H__