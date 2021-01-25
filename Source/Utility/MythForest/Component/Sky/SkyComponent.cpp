#include "SkyComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Transform/TransformComponent.h"
#include <utility>

using namespace PaintsNow;

SkyComponent::SkyComponent(const TShared<MeshResource>& res, const TShared<BatchComponent>& batch) : BaseClass(res, batch) {
	assert(res);
	Flag().fetch_or(COMPONENT_SHARED, std::memory_order_relaxed); // can be shared among different entities
}

TObject<IReflect>& SkyComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(atmosphereParameters);
	}

	return *this;
}

void SkyComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);
}

void SkyComponent::Uninitialize(Engine& engine, Entity* entity) {
	BaseClass::Uninitialize(engine, entity);
}

size_t SkyComponent::ReportGraphicMemoryUsage() const {
	return BaseClass::ReportGraphicMemoryUsage();
}
