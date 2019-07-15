#include "HeartVioliner.h"
#include "Queue.h"
#include "Clock.h"

using namespace PaintsNow;
using namespace PaintsNow::NsHeartVioliner;
using namespace PaintsNow::NsBridgeSunset;


HeartVioliner::HeartVioliner(IThread& thread, ITimer& base, BridgeSunset& b) : timerFactory(base), bridgeSunset(b) {}

TObject<IReflect>& HeartVioliner::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewClock)[ScriptMethod = "NewClock"];
		ReflectMethod(RequestSetClock)[ScriptMethod = "SetClock"];
		ReflectMethod(RequestAttach)[ScriptMethod = "Attach"];
		ReflectMethod(RequestDetach)[ScriptMethod = "Detach"];
		ReflectMethod(RequestStart)[ScriptMethod = "Start"];
		ReflectMethod(RequestPause)[ScriptMethod = "Pause"];
		ReflectMethod(RequestNow)[ScriptMethod = "Now"];
		ReflectMethod(RequestNewQueue)[ScriptMethod = "NewQueue"];
		ReflectMethod(RequestListen)[ScriptMethod = "Listen"];
		ReflectMethod(RequestPush)[ScriptMethod = "Push"];
		ReflectMethod(RequestPop)[ScriptMethod = "Pop"];
		ReflectMethod(RequestClear)[ScriptMethod = "Clear"];
	}

	return *this;
}


void HeartVioliner::RequestNewQueue(IScript::Request& request) {
	TShared<Queue> q = TShared<Queue>::From(new Queue());
	q->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
	bridgeSunset.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << q;
	request.UnLock();
}

void HeartVioliner::RequestStart(IScript::Request& request, IScript::Delegate<Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_THREAD_IN_LIBRARY(clock);

	clock->Play();
}

void HeartVioliner::RequestPause(IScript::Request& request, IScript::Delegate<Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_THREAD_IN_LIBRARY(clock);
	clock->Pause();
}

void HeartVioliner::RequestNow(IScript::Request& request, IScript::Delegate<Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_THREAD_IN_LIBRARY(clock);
	int64_t n = clock->Now();
	bridgeSunset.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << n;
	request.UnLock();
}

void HeartVioliner::RequestPush(IScript::Request& request, IScript::Delegate<Queue> queue, int64_t timeStamp, IScript::Request::Ref obj) {
	CHECK_REFERENCES(obj);
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);

	queue->Push(request, obj, timeStamp);
}

void HeartVioliner::RequestPop(IScript::Request& request, IScript::Delegate<Queue> queue, int64_t timeStamp) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);

	queue->ExecuteWithTimeStamp(request, timeStamp);
}

void HeartVioliner::RequestClear(IScript::Request& request, IScript::Delegate<Queue> queue) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);
	queue->Clear(request);
}

void HeartVioliner::RequestListen(IScript::Request& request, IScript::Delegate<Queue> queue, IScript::Request::Ref listener) {
	CHECK_REFERENCES_WITH_TYPE(listener, IScript::Request::FUNCTION);
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);
	queue->Listen(request, listener);
}

void HeartVioliner::RequestNewClock(IScript::Request& request, int64_t interval, int64_t start) {
	TShared<Clock> c = TShared<Clock>::From(new Clock(timerFactory, bridgeSunset, interval, start, true));
	c->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
	bridgeSunset.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << c;
	request.UnLock();
}

void HeartVioliner::RequestSetClock(IScript::Request& request, IScript::Delegate<Clock> clock, int64_t start) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_THREAD_IN_LIBRARY(clock);
	clock->SetClock(start);
}

void HeartVioliner::RequestAttach(IScript::Request& request, IScript::Delegate<Clock> clock, IScript::Delegate<Queue> queue) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(clock);
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(clock);
	CHECK_THREAD_IN_LIBRARY(queue);

	queue->Attach(clock.Get());
}

void HeartVioliner::RequestDetach(IScript::Request& request, IScript::Delegate<Queue> queue) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(queue);
	CHECK_THREAD_IN_LIBRARY(queue);

	queue->Detach();
}


/*
#include "../../General/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../General/Driver/Script/Lua/ZScriptLua.h"
#include "../../General/Driver/Timer/Freeglut/ZTimerFreeglut.h"

int HeartVioliner::main(int argc, char* argv[]) {
	ZThreadPthread thread;
	ZScriptLua script(thread);
	TFactory<ZTimerFreeglut, ITimer> fact;
	BridgeSunset bridgeSunset(thread, 3, 3);
	HeartVioliner hv(thread, fact, bridgeSunset);
	IScript::Request& request = script.GetDefaultRequest();
	IScript::Request::Ref ref = request.Load(String(
		"print('HeartVioliner::main()')\n"
		"local queue = HeartVioliner.CreateQueue()\n"
		"HeartVioliner.Listen(queue, function(v) print('Triggered! ' .. v) end)\n"
		"HeartVioliner.Push(queue, 0, 'hello')\n"
		"HeartVioliner.Push(queue, 1234, 'world')\n"
		"HeartVioliner.Push(queue, 3456, 1024)\n"
		"HeartVioliner.Pop(queue, 3333)\n"
		"print('STATUS' .. (queue == queue and 'YES' or 'NO'))\n"
		"print('IO' .. (io == nil and 'YES' or 'NO'))\n"
		));
	request << global << key("HeartVioliner") << hv << endtable;
	request << global << key("io") << nil << endtable;
	request << global << key("debug") << nil << endtable;
	request.Push();
	request.Call(sync, ref);
	request.Pop();
	request.Dereference(ref);
	return 0;
}
*/