// FieldMesh.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#pragma once
#include "../FieldComponent.h"

namespace PaintsNow {
	class FieldMesh : public FieldComponent::FieldBase {
	public:
		virtual Bytes operator [] (const Float3& position) const;
	};
}

