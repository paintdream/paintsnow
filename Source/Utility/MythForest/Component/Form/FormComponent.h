// FormComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/Interface/IType.h"

namespace PaintsNow {
	class FormComponent : public TAllocatedTiny<FormComponent, UniqueComponent<Component, SLOT_FORM_COMPONENT> > {
	public:
		FormComponent(const String& name);

		TShared<SharedTiny> GetCookie(void* key) const;
		void SetCookie(void* key, const TShared<SharedTiny>& tiny);
		void ClearCookies();

		const String& GetName() const;
		void SetName(const String& name);
		void SetName(rvalue<String> name);
		std::vector<String>& GetValues();
		const std::vector<String>& GetValues() const;

	protected:
		std::vector<std::key_value<void*, TShared<SharedTiny> > > cookies;
		String name; // entity name, maybe
		std::vector<String> values;
	};
}

