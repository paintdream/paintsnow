#include "RayForce.h"
#include "Tunnel.h"
#include "LibLoader.h"

using namespace PaintsNow;
using namespace PaintsNow::NsRayForce;

RayForce::RayForce(IThread& thread, NsSnowyStream::SnowyStream& s, NsBridgeSunset::BridgeSunset& bs) : snowyStream(s), bridgeSunset(bs) {}

TObject<IReflect>& RayForce::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewObject)[ScriptMethod = "NewObject"];
		ReflectMethod(RequestQueryInterface)[ScriptMethod = "QueryInterface"];
	}

	return *this;
}

void RayForce::RequestNewObject(IScript::Request& request, const String& path, const String& data) {
	IReflectObject* object = nullptr;
	IArchive& archive = snowyStream.GetInterfaces().archive;
	Bridge* bridge = nullptr;
	for (std::list<Bridge*>::iterator p = bridges.begin(); p != bridges.end(); ++p) {
		if ((object = (*p)->Create(request, archive, path, data))) {
			bridge = *p;
			break;
		}
	}

	if (object != nullptr) {
		assert(bridge != nullptr);
		// will be managed by script.
		TShared<Tunnel> tunnel = TShared<Tunnel>::From(new Tunnel(bridge, object));
		tunnel->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
		bridgeSunset.GetKernel().YieldCurrentWarp();

		request.DoLock();
		request << tunnel;
		request.UnLock();
	}
}

void RayForce::RequestQueryInterface(IScript::Request& request, IScript::Delegate<Tunnel> tunnel) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(tunnel);

	tunnel->Dump(request);
}

void RayForce::Register(Bridge* bridge) {
	bridges.emplace_back(bridge);
}
