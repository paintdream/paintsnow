#include "Queue.h"

using namespace PaintsNow;
using namespace PaintsNow::NsHeartVioliner;

Queue::Queue() {}

void Queue::Attach(TShared<Clock> c) {
	Detach();
	clock = c;
	clock->AddTicker(this, nullptr);
}

void Queue::Detach() {
	if (clock) {
		clock->RemoveTicker(this);
		clock = nullptr;
	}
}

void Queue::Clear(IScript::Request& request) {
	request.DoLock();
	while (!q.empty()) {
		Task task = q.top();
		request.Dereference(task.ref);
		q.pop();
	}
	request.UnLock();
}

void Queue::ScriptUninitialize(IScript::Request& request) {
	Clear(request);
	request.DoLock();
	
	for (std::list<IScript::Request::Ref>::reverse_iterator it = listeners.rbegin(); it != listeners.rend(); ++it) {
		request.Dereference(*it);
	}

	listeners.clear();
	request.UnLock();

	SharedTiny::ScriptUninitialize(request);
}

void Queue::Listen(IScript::Request& request, const IScript::Request::Ref& listener) {
	listeners.emplace_back(listener);
}

void Queue::Push(IScript::Request& request, IScript::Request::Ref& ref, int64_t timeStamp) {
	request.DoLock();
	q.push(Task(ref, timeStamp));
	request.UnLock();
}

void Queue::Execute(void* context) {
	int64_t timeStamp = clock->Now();
	NsBridgeSunset::BridgeSunset& bridgeSunset = *reinterpret_cast<NsBridgeSunset::BridgeSunset*>(context);
	IScript::Request& request = *bridgeSunset.AllocateRequest();
	ExecuteWithTimeStamp(request, timeStamp);
	bridgeSunset.FreeRequest(&request);
}

void Queue::Abort(void* context) {}

void Queue::ExecuteWithTimeStamp(IScript::Request& request, int64_t timeStamp) {
	request.DoLock();

	while (!q.empty()) {
		Task task = q.top();
		if (task.timeStamp > timeStamp) break;

		q.pop();
		Post(request, task.ref, timeStamp);
	}

	request.UnLock();
}

void Queue::Post(IScript::Request& request, IScript::Request::Ref ref, int64_t timeStamp) {
	// make a function call directly
	for (std::list<IScript::Request::Ref>::reverse_iterator it = listeners.rbegin(); it != listeners.rend(); ++it) {
		request.Push();
		request.Call(sync, *it, timeStamp, ref);
		request.Pop();
		assert(request.GetScript()->GetLockCount() == 1);
	}

	// release the reference
	request.Dereference(ref);
	assert(request.GetScript()->GetLockCount() == 1);
}