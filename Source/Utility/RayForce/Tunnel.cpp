#include "Tunnel.h"
#include "Bridge.h"
#include "ObjectDumper.h"

using namespace PaintsNow;
using namespace PaintsNow::NsRayForce;

Tunnel::Tunnel(Bridge* b, IReflectObject* h) : bridge(b), host(h) {
}

Tunnel::~Tunnel() {
	delete host;
}

TObject<IReflect>& Tunnel::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

void Tunnel::Dump(IScript::Request& request) {
	ObjectDumper dumper(request, *this, bridge->GetReflectMap());
	request.DoLock();
	request << begintable;
	// request << key("__tunnel") << this; // keep reference
	// bridge->Dump(request, *this, host);
	(*host)(dumper);
	request << endtable;
	request.UnLock();
}

void Tunnel::ForwardCall(const TProxy<>* p, IScript::Request& request) {
	bridge->Call(host, p, request);
}

Proxy& Tunnel::NewProxy(const TProxy<>* p) {
	proxy.emplace_back(Proxy(this, p));
	return proxy.back();
}

IReflectObject* Tunnel::GetHost() const {
	return host;
}