#include "ExplorerComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ExplorerComponent::ExplorerComponent() {
}

ExplorerComponent::~ExplorerComponent() {}

void ExplorerComponent::Initialize(Engine& engine, Entity* entity) {
	assert(!(Flag() & TINY_ACTIVATED));
	Component::Initialize(engine, entity);
}

void ExplorerComponent::Uninitialize(Engine& engine, Entity* entity) {
	assert((Flag() & TINY_ACTIVATED));
	Component::Uninitialize(engine, entity);
}
