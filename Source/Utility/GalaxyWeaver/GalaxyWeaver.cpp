#include "GalaxyWeaver.h"

using namespace PaintsNow;
using namespace PaintsNow::NsGalaxyWeaver;
using namespace PaintsNow::NsBridgeSunset;
using namespace PaintsNow::NsMythForest;

GalaxyWeaver::GalaxyWeaver(IThread& threadApi, ITunnel& net, NsBridgeSunset::BridgeSunset& sunset, NsSnowyStream::SnowyStream& ns, NsMythForest::MythForest& my) : network(net), bridgeSunset(sunset), snowyStream(ns), mythForest(my) {}

TObject<IReflect>& GalaxyWeaver::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewWeaver)[ScriptMethod = "NewWeaver"];
		ReflectMethod(RequestConnectEntity)[ScriptMethod = "ConnectEntity"];
		ReflectMethod(RequestSetWeaverRpcCallback)[ScriptMethod = "SetWeaverRpcCallback"];
		ReflectMethod(RequestSetWeaverConnectionCallback)[ScriptMethod = "SetWeaverConnectionCallback"];
		ReflectMethod(RequestStartWeaver)[ScriptMethod = "StartWeaver"];
		ReflectMethod(RequestStopWeaver)[ScriptMethod = "StopWeaver"];
		ReflectMethod(RequestCommitWeaverChanges)[ScriptMethod = "CommitWeaverChanges"];
	}

	return *this;
}

void GalaxyWeaver::RequestSetWeaverRpcCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

	weaver->SetRpcCallback(request, callback);
}

void GalaxyWeaver::RequestSetWeaverConnectionCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

	weaver->SetConnectionCallback(request, callback);
}

TShared<Weaver> GalaxyWeaver::RequestNewWeaver(IScript::Request& request, const String& config) {
	TShared<Weaver> weaver = TShared<Weaver>::From(new Weaver(bridgeSunset, snowyStream, mythForest, network, config));
	weaver->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
	bridgeSunset.GetKernel().YieldCurrentWarp();

	return weaver;
}

void GalaxyWeaver::RequestStartWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

	weaver->Start();
}

void GalaxyWeaver::RequestStopWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

	weaver->Stop();

}

void GalaxyWeaver::RequestCommitWeaverChanges(IScript::Request& request, IScript::Delegate<Weaver> weaver) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

}

void GalaxyWeaver::RequestConnectEntity(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Delegate<Entity> scene) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(weaver);
	CHECK_DELEGATE(scene);
	CHECK_THREAD_IN_LIBRARY(weaver);
	CHECK_THREAD_IN_LIBRARY(scene);

	assert(false);
}