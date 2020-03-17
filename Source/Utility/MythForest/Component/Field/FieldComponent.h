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

			enum {
				FIELDCOMPONENT_SIMPOLYGON = COMPONENT_CUSTOM_BEGIN,
				FIELDCOMPONENT_TEXTURE = COMPONENT_CUSTOM_BEGIN << 1,
				FIELDCOMPONENT_MESH = COMPONENT_CUSTOM_BEGIN << 2,
				FIELDCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 3,
			};

			enum SIMPOLYGON_TYPE {
				BOUNDING_BOX, BOUNDING_SPHERE, BOUNDING_CYLINDER,
			};

			enum TEXTURE_TYPE {
				TEXTURE_2D, TEXTURE_3D, 
			};

			Bytes operator [] (const Float3& position) const;

			class FieldBase : public TReflected<FieldBase, SharedTiny> {
			public:
				virtual Bytes operator [] (const Float3& position) const = 0;
			};

		protected:
			uint32_t subType;
			TShared<FieldBase> fieldImpl;
		};
	}
}


#endif // __FIELDCOMPONENT_H__