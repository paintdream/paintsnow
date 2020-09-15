// FieldComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	class FieldComponent : public TAllocatedTiny<FieldComponent, Component> {
	public:
		FieldComponent();
		~FieldComponent() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		Bytes operator [] (const Float3& position) const;

		class FieldBase : public TReflected<FieldBase, SharedTiny> {
		public:
			virtual Bytes operator [] (const Float3& position) const = 0;
		};

		void SetField(const TShared<FieldBase>& field);

	protected:
		uint32_t subType;
		TShared<FieldBase> fieldImpl;
	};
}

