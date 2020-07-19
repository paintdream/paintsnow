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
			TObject<IReflect>& operator () (IReflect& reflect) override;
			Bytes operator [] (const Float3& position) const;

			class FieldBase : public TReflected<FieldBase, SharedTiny> {
			public:
				virtual Bytes operator [] (const Float3& position) const = 0;
			};

			void SetField(TShared<FieldBase> field);

		protected:
			uint32_t subType;
			TShared<FieldBase> fieldImpl;
		};
	}
}


#endif // __FIELDCOMPONENT_H__