// FieldComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __FIELDCOMPONENT_H__
#define __FIELDCOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	namespace NsMythForest {
		class FieldComponent : public TAllocatedTiny<FieldComponent, Component> {
		public:
			FieldComponent();
			virtual ~FieldComponent();
		};
	}
}


#endif // __FIELDCOMPONENT_H__