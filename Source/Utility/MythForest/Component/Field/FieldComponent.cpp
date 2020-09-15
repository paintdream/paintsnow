#include "FieldComponent.h"

using namespace PaintsNow;

FieldComponent::FieldComponent() {
}

FieldComponent::~FieldComponent() {}

TObject<IReflect>& FieldComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(fieldImpl);
	}

	return *this;
}

Bytes FieldComponent::operator [] (const Float3& position) const {
	return (*fieldImpl)[position];
}

void FieldComponent::SetField(const TShared<FieldBase>& field) {
	fieldImpl = field;
}
