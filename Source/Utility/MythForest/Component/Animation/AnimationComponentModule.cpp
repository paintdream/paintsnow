#include "AnimationComponentModule.h"
#include "AnimationComponent.h"

using namespace PaintsNow;

AnimationComponentModule::AnimationComponentModule(Engine& engine) : BaseClass(engine) {
}

TObject<IReflect>& AnimationComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestAttach)[ScriptMethod = "Attach"];
		ReflectMethod(RequestDetach)[ScriptMethod = "Detach"];
		ReflectMethod(RequestPlay)[ScriptMethod = "Play"];
		ReflectMethod(RequestSetSpeed)[ScriptMethod = "SetSpeed"];
		ReflectMethod(RequestRegisterEvent)[ScriptMethod = "RegisterEvent"];
	}

	return *this;
}

TShared<AnimationComponent> AnimationComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<SkeletonResource> skeletonResource) {
	CHECK_REFERENCES_NONE();

	TShared<SkeletonResource> res = skeletonResource.Get();
	TShared<AnimationComponent> animationComponent = TShared<AnimationComponent>::From(allocator->New(res));
	animationComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	engine.GetKernel().YieldCurrentWarp();

	return animationComponent;
}

void AnimationComponentModule::RequestAttach(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& name, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(animationComponent);
	CHECK_DELEGATE(entity);

	animationComponent->Attach(name, entity.Get());
}

void AnimationComponentModule::RequestDetach(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(animationComponent);
	CHECK_DELEGATE(entity);

	animationComponent->Detach(entity.Get());
}

void AnimationComponentModule::RequestPlay(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& clipName, float startTime) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(animationComponent);

	animationComponent->Play(clipName, startTime);
}

void AnimationComponentModule::RequestSetSpeed(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, float speed) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(animationComponent);

	animationComponent->SetSpeed(speed);
}

void AnimationComponentModule::RequestRegisterEvent(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& identifier, const String& clipName, float time) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(animationComponent);

	animationComponent->RegisterEvent(identifier, clipName, time);
}