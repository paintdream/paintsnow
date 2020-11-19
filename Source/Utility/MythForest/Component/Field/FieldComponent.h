// FieldComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Space/SpaceComponent.h"

namespace PaintsNow {
	class FieldComponent : public TAllocatedTiny<FieldComponent, Component> {
	public:
		FieldComponent();
		~FieldComponent() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		Bytes operator [] (const Float3& position) const;

		class pure_interface FieldBase : public TReflected<FieldBase, SharedTiny> {
		public:
			virtual Bytes operator [] (const Float3& position) const = 0;
			virtual void PostEventForEntityTree(Entity* spaceComponent, Event& event, FLAG mask) const;
		};

		void SetField(const TShared<FieldBase>& field);
		void PostEvent(SpaceComponent* spaceComponent, Event& event, FLAG mask) const;

	protected:
		uint32_t subType;
		TShared<FieldBase> fieldImpl;
	};
}

