#include "Component.h"
#include "Entity.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"

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
	assert((Flag().load(std::memory_order_acquire) & COMPONENT_OVERRIDE_WARP) || entity->GetWarpIndex() == GetWarpIndex());
	Flag().fetch_or(Tiny::TINY_ACTIVATED, std::memory_order_relaxed);
}

void Component::Uninitialize(Engine& engine, Entity* entity) {
	Flag().fetch_and(~Tiny::TINY_ACTIVATED, std::memory_order_relaxed);
}

void Component::DispatchEvent(Event& event, Entity* entity) {}

void Component::UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) {}

Tiny::FLAG Component::GetEntityFlagMask() const {
	return 0;
}

Component::RaycastTask::RaycastTask() : clipOffDistanceSquared(FLT_MAX) {}

Component::RaycastTaskSerial::RaycastTaskSerial() {
	Flag().store(RAYCASTTASK_IGNORE_WARP, std::memory_order_relaxed);
	result.distance = FLT_MAX;
}

bool Component::RaycastTaskSerial::EmplaceResult(rvalue<Component::RaycastResult> item) {
	RaycastResult& r = item;
	if (result.distance > r.distance) {
		clipOffDistanceSquared = result.distance * result.distance; // update clip distance
		result = std::move(r);
		return true;
	} else {
		return false;
	}
}

Engine& Component::RaycastTaskWarp::GetEngine() {
	return engine;
}

Component::RaycastTaskWarp::RaycastTaskWarp(Engine& e, uint32_t m) : engine(e), maxCount(m) {
	pendingCount.store(0, std::memory_order_relaxed);
	results.resize(engine.GetKernel().GetWarpCount());
}

Component::RaycastTaskWarp::~RaycastTaskWarp() {

}

void Component::RaycastTaskWarp::AddPendingTask() {
	ReferenceObject();
	pendingCount.fetch_add(1, std::memory_order_relaxed);
}

void Component::RaycastTaskWarp::RemovePendingTask() {
	if (pendingCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
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

bool Component::RaycastTaskWarp::EmplaceResult(std::vector<RaycastResult>& result, rvalue<Component::RaycastResult> it) {
	Component::RaycastResult& item = it;
	static_assert(sizeof(uint32_t) == sizeof(float), "Must be IEEE 754 float32.");
	if (result.size() < maxCount) {
		if (maxCount == 1) {
			clipOffDistanceSquared = Math::Min(clipOffDistanceSquared, item.distance * item.distance);
		}
		result.emplace_back(std::move(item));
		return true;
	} else {
		for (size_t i = 0; i < result.size(); i++) {
			if (result[i].distance > item.distance) {
				if (maxCount == 1) {
					clipOffDistanceSquared = Math::Min(clipOffDistanceSquared, item.distance * item.distance);
				}

				result[i] = std::move(item);
				return true;
			}
		}

		return false;
	}
}

bool Component::RaycastTaskWarp::EmplaceResult(rvalue<Component::RaycastResult> item) {
	return EmplaceResult(results[engine.GetKernel().GetCurrentWarpIndex()], std::move(item));
}

float Component::Raycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio) const { return ratio; }

void Component::RaycastForEntity(RaycastTask& task, Float3Pair& ray, Entity* entity) {
	OPTICK_EVENT();
	assert(!(entity->Flag().load(std::memory_order_acquire) & TINY_MODIFIED));

	TVector<float, 2> intersect = Math::IntersectBox(entity->GetKey(), ray);
	if (intersect[1] < 0.0f || intersect[0] > intersect[1])
		return;

	// evaluate possible distance
	if (task.clipOffDistanceSquared != FLT_MAX) {
		float nearest = Math::SquareLength(entity->GetKey().second - ray.first) - Math::SquareLength(entity->GetKey().second - entity->GetKey().first) * 0.5f;
		if (nearest >= task.clipOffDistanceSquared)
			return;
	}

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
