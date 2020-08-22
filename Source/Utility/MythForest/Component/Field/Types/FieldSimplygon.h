// FieldSimplygon.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#pragma once
#include "../FieldComponent.h"

namespace PaintsNow {
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

