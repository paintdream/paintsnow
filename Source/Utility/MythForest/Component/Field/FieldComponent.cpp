#include "FieldComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

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

void FieldComponent::SetField(TShared<FieldBase> field) {
	fieldImpl = field;
}
