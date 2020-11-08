#include "Looper.h"

using namespace PaintsNow;

Looper::Looper(BridgeSunset& bs, INetwork& nt) : network(nt), bridgeSunset(bs) {}

Looper::~Looper() {
}

struct AsyncInfo {
	AsyncInfo(IScript& s, Looper* l) : script(s), looper(l) {}
	IScript& script;
	Looper* looper;
};

bool Looper::ActivateRoutine(IThread::Thread* thread, size_t context) {
	AsyncInfo* info = reinterpret_cast<AsyncInfo*>(context);
	Activate();
	IScript::Request& request = info->script.GetDefaultRequest();
	info->looper->ReleaseObject();
	delete info;

	return true;
}

void Looper::AsyncActivate(IScript::Request& request) {
	// hold self reference
	IThread& threadApi = bridgeSunset.GetThreadApi();
	request.DoLock();
	ReferenceObject();
	AsyncInfo* info = new AsyncInfo(*request.GetScript(), this);
	request.UnLock();
	threadApi.NewThread(Wrap(this, &Looper::ActivateRoutine), reinterpret_cast<size_t>(info));
}

String Looper::EventToString(INetwork::EVENT event) {
	String target = "Close";
	switch (event) {
	case INetwork::CONNECTED:
		target = "Connected";
		break;
	case INetwork::TIMEOUT:
		target = "Timeout";
		break;
	case INetwork::READ:
		target = "Read";
		break;
	case INetwork::WRITE:
		target = "Write";
		break;
	case INetwork::CLOSE:
		target = "Close";
		break;
	case INetwork::ABORT:
		target = "Error";
		break;
	case INetwork::CUSTOM:
		target = "Custom";
		break;
	}

	return target;
}
