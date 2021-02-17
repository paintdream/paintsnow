#include "BridgeComponent.h"
#include "../../Entity.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

BridgeComponent::BridgeComponent(const TShared<Component>& targetComponent) : hostEntity(nullptr) {
	Flag().fetch_or(TINY_UNIQUE, std::memory_order_relaxed);
	assert(targetComponent);
	assert(targetComponent->GetWarpIndex() == GetWarpIndex());
}

void BridgeComponent::Clear(Engine& engine) {
	assert(hostEntity != nullptr);
	hostEntity->RemoveComponent(engine, this);
	hostEntity = nullptr;
}

void BridgeComponent::DispatchEvent(Event& event, Entity* entity) {
	// broadcast all events.
	targetComponent->DispatchEvent(event, entity);
}

void BridgeComponent::Initialize(Engine& engine, Entity* entity) {
	hostEntity = entity;
}

void BridgeComponent::Uninitialize(Engine& engine, Entity* entity) {
	hostEntity = nullptr;
}

Entity* BridgeComponent::GetHostEntity() const {
	return hostEntity;
}
