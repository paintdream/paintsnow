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
			enum SIMPOLYGON_TYPE {
				BOUNDING_BOX, BOUNDING_SPHERE, BOUNDING_CYLINDER,
			};

			FieldSimplygon(SIMPOLYGON_TYPE type);

			virtual Bytes operator [] (const Float3& position) const override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		protected:
			SIMPOLYGON_TYPE type;
		};
	}
}

#endif // __FIELDSIMPLYGON_H__