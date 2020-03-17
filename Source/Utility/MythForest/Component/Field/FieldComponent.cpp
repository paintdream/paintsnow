#include "FieldComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

FieldComponent::FieldComponent() {
}

FieldComponent::~FieldComponent() {}

Bytes FieldComponent::operator [] (const Float3& position) const {
	return (*fieldImpl)[position];
}
