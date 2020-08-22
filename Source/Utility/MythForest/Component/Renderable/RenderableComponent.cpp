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

