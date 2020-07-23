#include "ProxyStub.h"

using namespace PaintsNow;
using namespace PaintsNow::NsGalaxyWeaver;

ProxyStub::ProxyStub(IThread& thread, ITunnel& tunnel, const String& entry, const TWrapper<void, IScript::Request&, bool, RemoteProxy::STATUS, const String&>& statusHandler) : remoteProxy(thread, tunnel, Wrap(this, &ProxyStub::StaticCreate), entry, statusHandler) {

}

void ProxyStub::ScriptInitialize(IScript::Request& request) {
	// Avoid cycle reference
	if (request.GetScript() != &remoteProxy) {
		WarpTiny::ScriptInitialize(request);
	}
}

void ProxyStub::ScriptUninitialize(IScript::Request& request) {
	// Avoid cycle reference
	if (request.GetScript() != &remoteProxy) {
		WarpTiny::ScriptUninitialize(request);
	}
}

IScript::Object* ProxyStub::StaticCreate(const String& entry) {
	return this;
}