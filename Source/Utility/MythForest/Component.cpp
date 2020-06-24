#include "Component.h"
#include "Entity.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

uint32_t Component::GetQuickUniqueID() const {
	return ~(uint32_t)0;
}

void Component::Initialize(Engine& engine, Entity* entity) {
	SetWarpIndex(entity->GetWarpIndex());
	Flag().fetch_or(Tiny::TINY_ACTIVATED, std::memory_order_acquire);
}

void Component::Uninitialize(Engine& engine, Entity* entity) {
}

void Component::DispatchEvent(Event& event, Entity* entity) {}

void Component::UpdateBoundingBox(Engine& engine, Float3Pair& box) {}

Tiny::FLAG Component::GetEntityFlagMask() const {
	return 0;
}