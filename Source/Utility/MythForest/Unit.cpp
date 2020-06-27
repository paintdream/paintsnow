#include "Unit.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

String Unit::GetDescription() const {
	std::stringstream ss;
	ss << GetUnique()->GetSubName() << "(" << std::hex << (size_t)this << ")";
	return ss.str();
}

TObject<IReflect>& Unit::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}


MetaUnitIdentifier::MetaUnitIdentifier() {}

TObject<IReflect>& MetaUnitIdentifier::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

bool MetaUnitIdentifier::Read(IStreamBase& streamBase, void* ptr) const {
	// TODO:
	assert(false);
	return true;
}

bool MetaUnitIdentifier::Write(IStreamBase& streamBase, const void* ptr) const {
	// TODO:
	assert(false);
	return true;
}

const String& MetaUnitIdentifier::GetUniqueName() const {
	return uniqueName;
}
