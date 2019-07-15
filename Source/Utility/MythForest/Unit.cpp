#include "Unit.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

bool Unit::EmplaceRaycastResult(std::vector<RaycastResult>& results, uint32_t maxCount, const RaycastResult& item) {
	if (results.size() < maxCount) {
		results.emplace_back(item);
		return true;
	} else {
		for (size_t i = 0; i < results.size(); i++) {
			if (results[i].distance > item.distance) {
				results[i] = item;
				return true;
			}
		}

		return false;
	}
}

void Unit::Raycast(std::vector<RaycastResult>& results, Float3Pair& ray, uint32_t maxCount, IReflectObject* filter) const {}

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
