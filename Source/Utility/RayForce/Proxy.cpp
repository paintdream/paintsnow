#include "Proxy.h"
#include "Tunnel.h"

using namespace PaintsNow;
using namespace PaintsNow::NsRayForce;

Proxy::Proxy(Tunnel* host, const TProxy<>* p) : hostTunnel(host), routine(p) {}

void Proxy::OnCall(IScript::Request& request) {
	hostTunnel->ForwardCall(routine, request);
}