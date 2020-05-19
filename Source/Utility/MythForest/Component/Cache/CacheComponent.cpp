#include "CacheComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

CacheComponent::CacheComponent() {}

void CacheComponent::PushObjects(const std::vector<TShared<SharedTiny> >& objects) {
	if (cachedObjects.empty()) {
		cachedObjects = objects;
	} else {
		for (size_t i = 0; i < objects.size(); i++) {
			cachedObjects.push_back(objects[i]);
		}
	}
}

void CacheComponent::ClearObjects() {
	cachedObjects.clear();
}