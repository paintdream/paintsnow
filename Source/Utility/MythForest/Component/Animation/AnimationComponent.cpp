#include "AnimationComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Event/EventListenerComponent.h"
#include "../../MythForest.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

AnimationComponent::AnimationComponent(TShared<SkeletonResource> resource) : skeletonResource(resource), animationTime(0), speed(1), clipIndex(0), boneMatrixBuffer(nullptr) {
	// can be shared between entities
	Flag().fetch_or(Component::COMPONENT_SHARED, std::memory_order_acquire);
	const IAsset::BoneAnimation& boneAnimation = resource->GetBoneAnimation();
	boneMatrices.resize(boneAnimation.joints.size());
	eventData.resize(boneAnimation.clips.size());
}

AnimationComponent::~AnimationComponent() {
}

Tiny::FLAG AnimationComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT | Entity::ENTITY_HAS_RENDERCONTROL;
}

TShared<SkeletonResource> AnimationComponent::GetSkeletonResource() {
	return skeletonResource;
}

void AnimationComponent::SetSpeed(float s) {
	speed = s;
}

void AnimationComponent::RegisterEvent(const String& id, const String& clipName, float timeStamp) {
	const IAsset::BoneAnimation& boneAnimation = skeletonResource->GetBoneAnimation();
	for (size_t i = 0; i < boneAnimation.clips.size(); i++) {
		if (boneAnimation.clips[i].name == clipName) {
			EventData data;
			data.identifier = id;
			data.timeStamp = timeStamp;
			eventData[i].emplace_back(std::move(data));
		}
	}
}

void AnimationComponent::Play(const String& clipName, float startTime) {
	// find clip
	const IAsset::BoneAnimation& boneAnimation = skeletonResource->GetBoneAnimation();
	for (size_t i = 0; i < boneAnimation.clips.size(); i++) {
		if (boneAnimation.clips[i].name == clipName) {
			clipIndex = safe_cast<uint32_t>(i);
			animationTime = startTime;
			break;
		}
	}
}

void AnimationComponent::Detach(TShared<Entity> entity) {
	assert(entity->GetWarpIndex() == GetWarpIndex());
	for (size_t i = 0; i < mountPoints.size(); i++) {
		if (mountPoints[i].second == entity) {
			mountPoints.erase(mountPoints.begin() + i);
			return;
		}
	}
}

void AnimationComponent::Attach(const String& name, TShared<Entity> entity) {
	assert(entity->GetWarpIndex() == GetWarpIndex());
	const IAsset::BoneAnimation& boneAnimation = skeletonResource->GetBoneAnimation();
	for (size_t i = 0; i < boneAnimation.joints.size(); i++) {
		if (boneAnimation.joints[i].name == name) {
			mountPoints.emplace_back(std::make_pair((uint32_t)safe_cast<uint32_t>(i), entity));
		}
	}
}

void AnimationComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);
}

void AnimationComponent::Uninitialize(Engine& engine, Entity* entity) {
	skeletonResource->ClearBoneMatrixBuffer(engine.interfaces.render, engine.mythForest.GetWarpResourceQueue(), boneMatrixBuffer);
	BaseClass::Uninitialize(engine, entity);
}

IRender::Resource* AnimationComponent::AcquireBoneMatrixBuffer(IRender& render, IRender::Queue* queue) {
	if (boneMatrixBuffer == nullptr) {
		skeletonResource->UpdateBoneMatrixBuffer(render, queue, boneMatrixBuffer, boneMatrices);
	}

	return boneMatrixBuffer;
}

void AnimationComponent::DispatchEvent(Event& event, Entity* entity) {
	if (event.eventID == Event::EVENT_TICK) {
		EventListenerComponent* eventListenerComponent = static_cast<EventListenerComponent*>(event.eventDetail());
		assert(eventListenerComponent != nullptr);
	
		bool repeat = !!(Flag() & ANIMATIONCOMPONENT_REPEAT);

		// update bone matrices
		float before = animationTime;
		animationTime += (float)eventListenerComponent->GetTickDeltaTime();
		const std::vector<EventData>& events = eventData[clipIndex];
		Event triggerEvent(event.engine, Event::EVENT_CUSTOM, entity, this);

		for (size_t k = 0; k < events.size(); k++) {
			const EventData& data = events[k];
			if (data.timeStamp >= before && data.timeStamp < animationTime) {
				// trigger event
				triggerEvent.eventMeta = data.identifier;
				entity->PostEvent(triggerEvent);
			}
		}

		animationTime = skeletonResource->Snapshot(boneMatrices, clipIndex, animationTime, repeat);
		const std::vector<MatrixFloat4x4>& offsetMatrices = skeletonResource->GetOffsetTransforms();

		for (size_t i = 0; i < mountPoints.size(); i++) {
			std::pair<uint32_t, TShared<Entity> >& p = mountPoints[i];
			assert(p.second->GetWarpIndex() == GetWarpIndex());
			TransformComponent* transformComponent = p.second->GetUniqueComponent(UniqueType<TransformComponent>());
			assert(p.first < mountPoints.size());
			transformComponent->SetTransform(offsetMatrices[p.first] * boneMatrices[p.first]);
		}
	}
}