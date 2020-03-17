// FieldTexture.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#ifndef __FIELDTEXTURE_H__
#define __FIELDTEXTURE_H__

#include "../FieldComponent.h"

namespace PaintsNow {
	namespace NsMythForest {
		class FieldTexture : public FieldComponent::FieldBase {
		public:
			virtual Bytes operator [] (const Float3& position) const;
		};
	}
}

#endif // __FIELDTEXTURE_H__