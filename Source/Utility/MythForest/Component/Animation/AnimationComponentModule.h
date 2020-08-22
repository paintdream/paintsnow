// AnimationComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "AnimationComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class AnimationComponentModule : public TReflected<AnimationComponentModule, ModuleImpl<AnimationComponent> > {
	public:
		AnimationComponentModule(Engine& engine);

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<AnimationComponent> RequestNew(IScript::Request& request, IScript::Delegate<SkeletonResource> skeletonResource);
		void RequestPlay(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& clipName, float startTime);
		void RequestSetSpeed(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, float speed);
		void RequestAttach(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& name, IScript::Delegate<Entity> entity);
		void RequestDetach(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, IScript::Delegate<Entity> entity);
		void RequestRegisterEvent(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& identifier, const String& clipName, float time);
	};
}

