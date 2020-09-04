#include "Component.h"
#include "Entity.h"

using namespace PaintsNow;

uint32_t Component::GetQuickUniqueID() const {
	return ~(uint32_t)0;
}

Entity* Component::GetHostEntity() const {
	return nullptr;
}

const String& Component::GetAliasedTypeName() const {
	static String emptyName;
	return emptyName;
}

void Component::Initialize(Engine& engine, Entity* entity) {
	assert((Flag() & COMPONENT_LOCALIZED_WARP) || entity->GetWarpIndex() == GetWarpIndex());
	Flag().fetch_or(Tiny::TINY_ACTIVATED, std::memory_order_acquire);
}

void Component::Uninitialize(Engine& engine, Entity* entity) {
	Flag().fetch_and(~Tiny::TINY_ACTIVATED, std::memory_order_release);
}

void Component::DispatchEvent(Event& event, Entity* entity) {}

void Component::UpdateBoundingBox(Engine& engine, Float3Pair& box) {}

Tiny::FLAG Component::GetEntityFlagMask() const {
	return 0;
}

Engine& Component::RaycastTask::GetEngine() {
	return engine;
}

Component::RaycastTask::RaycastTask(Engine& e, uint32_t m) : engine(e), maxCount(m) {
	pendingCount.store(0, std::memory_order_relaxed);
	results.resize(engine.GetKernel().GetWarpCount());
}

Component::RaycastTask::~RaycastTask() {

}

void Component::RaycastTask::AddPendingTask() {
	ReferenceObject();
	pendingCount.fetch_add(1, std::memory_order_acquire);
}

void Component::RaycastTask::RemovePendingTask() {
	if (pendingCount.fetch_sub(1, std::memory_order_release) == 1) {
		std::vector<RaycastResult> finalResult;

		uint32_t collectedWarpCount = 0;
		for (size_t i = 0; i < results.size() && collectedWarpCount < 2; i++) {
			collectedWarpCount += results[i].size() != 0;
		}

		if (collectedWarpCount < 2) {
			for (size_t i = 0; i < results.size(); i++) {
				if (results[i].size() != 0) {
					Finish(std::move(results[i]));
					ReleaseObject();
					return;
				}
			}
		} else {
			// Need to merge results from different warps
			for (size_t i = 0; i < results.size(); i++) {
				std::vector<RaycastResult>& result = results[i];
				if (!result.empty()) {
					if (finalResult.empty()) {
						finalResult = std::move(result);
					} else {
						for (size_t k = 0; k < result.size(); k++) {
							EmplaceResult(finalResult, std::move(result[k]));
						}
					}
				}
			}
		}
		
		Finish(std::move(finalResult));
	}

	ReleaseObject();
}

bool Component::RaycastTask::EmplaceResult(std::vector<RaycastResult>& result, rvalue<Component::RaycastResult> it) {
	Component::RaycastResult& item = it;
	if (result.size() < maxCount) {
		result.emplace_back(std::move(item));
		return true;
	} else {
		for (size_t i = 0; i < result.size(); i++) {
			if (result[i].distance > item.distance) {
				result[i] = std::move(item);
				return true;
			}
		}

		return false;
	}
}

bool Component::RaycastTask::EmplaceResult(rvalue<Component::RaycastResult> item) {
	return EmplaceResult(results[engine.GetKernel().GetCurrentWarpIndex()], std::move(item));
}

float Component::Raycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const { return ratio; }

void Component::RaycastForEntity(RaycastTask& task, Float3Pair& ray, Entity* entity) {
	assert(!(entity->Flag() & TINY_MODIFIED));
	if (!Math::IntersectBox(entity->GetKey(), ray))
		return;

	Float3Pair newRay = ray;
	std::vector<RaycastResult> newResults;
	const std::vector<Component*>& components = entity->GetComponents();
	float ratio = 1.0f;

	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			ratio = component->Raycast(task, newRay, entity, ratio);
		}
	}
}
