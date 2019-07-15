// AnimationComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __ANIMATIONCOMPONENT_H__
#define __ANIMATIONCOMPONENT_H__

#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/SkeletonResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		class AnimationComponent : public TAllocatedTiny<AnimationComponent, UniqueComponent<Component, SLOT_ANIMATION_COMPONENT> > {
		public:
			enum {
				ANIMATIONCOMPONENT_REPEAT = COMPONENT_CUSTOM_BEGIN,
			};

			AnimationComponent(TShared<NsSnowyStream::SkeletonResource> skeletonResource);
			virtual ~AnimationComponent();
			virtual void Initialize(Engine& engine, Entity* entity) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
			virtual void DispatchEvent(Event& event, Entity* entity) override;
			virtual FLAG GetEntityFlagMask() const;

			IRender::Resource* AcquireBoneMatrixBuffer(IRender::Queue* queue);

			void Attach(const String& name, TShared<Entity> entity);
			void Detach(TShared<Entity> entity);
			void Play(const String& clipName, float startTime);
			void RegisterEvent(const String& identifier, const String& clipName, float timeStamp);
			void SetSpeed(float speed);
			TShared<NsSnowyStream::SkeletonResource> GetSkeletonResource();

		private:
			struct EventData {
				String identifier;
				float timeStamp;
			};

		private:
			TShared<NsSnowyStream::SkeletonResource> skeletonResource;
			std::vector<MatrixFloat4x4> boneMatrices;
			std::vector<std::pair<uint32_t, TShared<Entity> > > mountPoints;
			std::vector<std::vector<EventData> > eventData;
			IRender::Resource* boneMatrixBuffer;
			uint32_t clipIndex;
			float animationTime;
			float speed;
		};
	}
}


#endif // __ANIMATIONCOMPONENT_H__
