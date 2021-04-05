#include "RenderableComponent.h"
#include "../../Entity.h"

using namespace PaintsNow;

RenderableComponent::RenderableComponent() {}

Tiny::FLAG RenderableComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_RENDERABLE;
}

size_t RenderableComponent::ReportGraphicMemoryUsage() const {
	return 0;
}

const std::vector<TShared<RenderPolicy> >& RenderableComponent::GetRenderPolicies() const {
	return renderPolicies;
}

void RenderableComponent::AddRenderPolicy(const TShared<RenderPolicy>& renderPolicy) {
	renderPolicies.emplace_back(renderPolicy);
}

void RenderableComponent::RemoveRenderPolicy(const TShared<RenderPolicy>& renderPolicy) {
	for (size_t i = 0; i < renderPolicies.size(); i++) {
		if (renderPolicy == renderPolicies[i]) {
			renderPolicies.erase(renderPolicies.begin() + i);
			break;
		}
	}
}

TObject<IReflect>& RenderableComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(renderPolicies)[Runtime];
	}

	return *this;
}
