// FieldSimplygon.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#ifndef __FIELDSIMPLYGON_H__
#define __FIELDSIMPLYGON_H__

#include "../FieldComponent.h"

namespace PaintsNow {
	namespace NsMythForest {
		class FieldSimplygon : public FieldComponent::FieldBase {
		public:
			virtual Bytes operator [] (const Float3& position) const;
		};
	}
}

#endif // __FIELDSIMPLYGON_H__