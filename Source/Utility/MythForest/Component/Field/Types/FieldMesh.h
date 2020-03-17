// FieldMesh.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#ifndef __FIELDMESH_H__
#define __FIELDMESH_H__

#include "../FieldComponent.h"

namespace PaintsNow {
	namespace NsMythForest {
		class FieldMesh : public FieldComponent::FieldBase {
		public:
			virtual Bytes operator [] (const Float3& position) const;
		};
	}
}

#endif // __FIELDMESH_H__