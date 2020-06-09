#include "Listener.h"
#include "Connection.h"

using namespace PaintsNow;
using namespace PaintsNow::NsEchoLegend;
using namespace PaintsNow::NsBridgeSunset;

Listener::Listener(BridgeSunset& bs, INetwork& nt, WorkDispatcher* disp, IScript::Request::Ref peventHandler, IScript::Request::Ref pcallback, IScript::Request::Ref pconnectCallback, const String& ip, bool h, bool mode) : network(nt), bridgeSunset(bs), listener(nullptr), dispatcher(disp), httpd(nullptr), eventHandler(peventHandler), callback(pcallback), connectCallback(pconnectCallback) {
//	, http(h), activated(false), packetMode(mode)
	if (h) {
		Flag() |= LISTENER_HTTP;
	}

	if (mode) {
		Flag() |= LISTENER_PACKET_MODE;
	}

	listener = network.OpenListener(dispatcher->GetDispatcher(), Wrap(this, &Listener::OnEvent), Wrap(this, &Listener::OnAccept), ip);
	if (listener != nullptr) {
		if (h) {
			httpd = network.OpenHttpd(listener, Wrap(this, &Listener::OnAcceptHttp));
		}
	}
}

bool Listener::IsValid() const {
	return listener != nullptr && (!(Flag() & LISTENER_HTTP) || httpd != nullptr);
}

String Listener::GetAddress() {
	String info;
	network.GetListenerInfo(listener, info);
	
	return info;
}

void Listener::OnEvent(INetwork::EVENT event) {
	bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(eventHandler, this, Looper::EventToString(event)));
}

Listener::~Listener() {
	if (httpd != nullptr) {
		network.CloseHttpd(httpd);
	}

	network.CloseListener(listener);
}

const TWrapper<void, INetwork::EVENT> Listener::OnAccept(INetwork::Connection* connection) {
	IScript::Request& r = bridgeSunset.GetScript().GetDefaultRequest();
	r.DoLock();
	IScript::Request::Ref ref = r.Reference(connectCallback);
	r.UnLock();

	WorkDispatcher* disp = dispatcher();
	INetwork::HttpRequest* req = (INetwork::HttpRequest*)nullptr;
	bool packetMode = !!(Flag() & LISTENER_PACKET_MODE);
	TShared<Connection> c = TShared<Connection>::From(new Connection(bridgeSunset, network, disp, ref, "", connection, false, req, false, packetMode));

	IScript::BaseDelegate d(this);
	IScript::BaseDelegate dc(c());
	bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(callback, this, c));
	return Wrap(c(), &Connection::OnEvent);
}

void Listener::OnAcceptHttp(INetwork::Connection* connection, INetwork::HttpRequest* httpRequest) {
	IScript::Request& r = bridgeSunset.GetScript().GetDefaultRequest();
	r.DoLock();
	IScript::Request::Ref ref = r.Reference(connectCallback);
	r.UnLock();

	WorkDispatcher* disp = dispatcher();
	bool packetMode = !!(Flag() & LISTENER_PACKET_MODE);
	TShared<Connection> c = TShared<Connection>::From(new Connection(bridgeSunset, network, disp, ref, "", connection, true, httpRequest, true, packetMode));

	bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(callback, this, c));
}


void Listener::ScriptUninitialize(IScript::Request& request) {
	request.DoLock();
	if (callback) {
		request.Dereference(callback);
	}

	if (eventHandler) {
		request.Dereference(eventHandler);
	}

	if (connectCallback) {
		request.Dereference(connectCallback);
	}
	request.UnLock();

	SharedTiny::ScriptUninitialize(request);
}

bool Listener::Activate() {
	bool ret = false;
	if (!(Flag() & TINY_ACTIVATED)) {
		Flag() |= TINY_ACTIVATED;
		if (Flag() & LISTENER_HTTP) {
			ret = network.ActivateHttpd(httpd);
		} else {
			ret = network.ActivateListener(listener);
		}
		Flag() &= ~TINY_ACTIVATED;
	}

	return ret;
}

void Listener::Deactivate() {
	if (Flag() & TINY_ACTIVATED) {
		network.DeactivateListener(listener);
		Flag() &= ~TINY_ACTIVATED;
	}
}
