#include "Controller.h"
#include "Weaver.h"

using namespace PaintsNow;
using namespace PaintsNow::NsGalaxyWeaver;

Controller::Controller(IThread& threadApi, ITunnel& tunnel, const String& entry) : BaseClass(threadApi, tunnel, entry) {}
Controller::~Controller() {}

TObject<IReflect>& Controller::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(CheckVersion)[ScriptRemoteMethod(&Prototype::RpcCheckVersion)];
		ReflectProperty(PostResource)[ScriptRemoteMethod(&Prototype::RpcPostResource)];
		ReflectProperty(Complete)[ScriptRemoteMethod(&Prototype::RpcComplete)];
		ReflectProperty(PostEntity)[ScriptRemoteMethod(&Prototype::RpcPostEntity)];
		ReflectProperty(PostEntityGroup)[ScriptRemoteMethod(&Prototype::RpcPostEntityGroup)];
		ReflectProperty(PostEntityComponent)[ScriptRemoteMethod(&Prototype::RpcPostEntityComponent)];
		ReflectProperty(PostModelComponent)[ScriptRemoteMethod(&Prototype::RpcPostModelComponent)];
		ReflectProperty(PostModelComponentMaterial)[ScriptRemoteMethod(&Prototype::RpcPostModelComponentMaterial)];
		ReflectProperty(PostTransformComponent)[ScriptRemoteMethod(&Prototype::RpcPostTransformComponent)];
		ReflectProperty(PostSpaceComponent)[ScriptRemoteMethod(&Prototype::RpcPostSpaceComponent)];
		ReflectProperty(PostEnvCubeComponent)[ScriptRemoteMethod(&Prototype::RpcPostEnvCubeComponent)];
		ReflectProperty(UpdateView)[ScriptRemoteMethod(&Prototype::RpcUpdateView)];
	}

	return *this;
}