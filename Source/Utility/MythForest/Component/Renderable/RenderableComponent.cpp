#include "RenderableComponent.h"
#include "../../Entity.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

RenderableComponent::RenderableComponent() {}

Tiny::FLAG RenderableComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_RENDERABLE;
}

