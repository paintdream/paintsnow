#include "BridgeSunset.h"

using namespace PaintsNow;

BridgeSunset::BridgeSunset(IThread& t, IScript& s, uint32_t threadCount, uint32_t warpCount) : ISyncObject(t), RequestPool(s, warpCount), threadPool(t, threadCount), kernel(threadPool, warpCount) {
	if (!threadPool.IsInitialized()) {
		threadPool.Initialize();
	}

	script.DoLock();
	for (uint32_t k = 0; k < threadPool.GetThreadCount(); k++) {
		threadPool.SetThreadContext(k, this);
	}

	// register script dispatcher hook
	origDispatcher = script.GetDispatcher();
	script.SetDispatcher(Wrap(this, &BridgeSunset::ContinueScriptDispatcher));
	script.UnLock();
}

BridgeSunset::~BridgeSunset() {
	assert(threadPool.IsInitialized());
	threadPool.Uninitialize();

	IScript::Request& mainRequest = script.GetDefaultRequest();
	assert(script.GetDispatcher() == Wrap(this, &BridgeSunset::ContinueScriptDispatcher));
	script.SetDispatcher(origDispatcher);
	Clear(); // clear request pool
}

void BridgeSunset::ScriptInitialize(IScript::Request& request) {
	Library::ScriptInitialize(request);
}

void BridgeSunset::ScriptUninitialize(IScript::Request& request) {
	Library::ScriptUninitialize(request);
}

TObject<IReflect>& BridgeSunset::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewGraph)[ScriptMethod = "NewGraph"];
		ReflectMethod(RequestQueueGraphRoutine)[ScriptMethod = "QueueGraphRoutine"];
		ReflectMethod(RequestConnectGraphRoutine)[ScriptMethod = "ConnectGraphRoutine"];
		ReflectMethod(RequestExecuteGraph)[ScriptMethod = "ExecuteGraph"];
		ReflectMethod(RequestQueueRoutine)[ScriptMethod = "QueueRoutine"];
		ReflectMethod(RequestGetWarpCount)[ScriptMethod = "GetWarpCount"];
	}

	return *this;
}

Kernel& BridgeSunset::GetKernel() {
	return kernel;
}

void BridgeSunset::Dispatch(ITask* task) {
	threadPool.Push(task);
}

class ScriptContinuer : public TaskOnce {
public:
	ScriptContinuer(const TWrapper<void, IScript::Request&>& c, IScript::Request& r) : continuer(c), request(r) {
		token.store(1, std::memory_order_relaxed);
	}

	inline void Complete() {
		token.store(0, std::memory_order_release);
	}

	void Execute(void* context) override {
		BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
		request.DoLock();
		bridgeSunset.ContinueScriptDispatcher(request, nullptr, 0, continuer);
		request.UnLock();
		Complete();
	}

	void Abort(void* context) override {
		// do not block script processing
		Execute(context);
		// Complete();
	}

	TWrapper<void, IScript::Request&> continuer;
	std::atomic<int32_t> token;
	IScript::Request& request;
};

void BridgeSunset::ContinueScriptDispatcher(IScript::Request& request, IHost* host, size_t paramCount, const TWrapper<void, IScript::Request&>& continuer) {
	// check if current warp is yielded
	static thread_local uint32_t stackWarpIndex = 0;
	uint32_t warpIndex = kernel.GetCurrentWarpIndex();
	if (warpIndex == ~(uint32_t)0) warpIndex = stackWarpIndex;
	else stackWarpIndex = warpIndex;
	Kernel::SubTaskQueue& queue = kernel.taskQueueGrid[warpIndex];

	if (queue.PreemptExecution()) {
		continuer(request);
		queue.YieldExecution();
	} else {
		request.UnLock();
		size_t threadIndex = threadPool.GetCurrentThreadIndex();
		// Only fails on destructing
		ScriptContinuer scriptContinuer(continuer, request);
		queue.Push(safe_cast<uint32_t>(threadIndex), &scriptContinuer, nullptr);
		queue.Flush(threadPool);

		// wait for it finishes
		while (scriptContinuer.token.load(std::memory_order_acquire) != 0) {
			threadPool.PollRoutine(safe_cast<uint32_t>(threadIndex));
		}

		request.DoLock();
	}

	stackWarpIndex = warpIndex;
}

void BridgeSunset::RequestQueueRoutine(IScript::Request& request, IScript::Delegate<WarpTiny> unit, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(unit);
	GetKernel().YieldCurrentWarp();

	if (GetKernel().GetCurrentWarpIndex() != unit->GetWarpIndex()) {
		GetKernel().QueueRoutine(unit.Get(), CreateTaskScriptOnce(callback));
	} else {
		request.DoLock();
		request.Call(deferred, callback); // use async call to prevent stack depth increase
		request.Dereference(callback);
		request.UnLock();
	}
}

uint32_t BridgeSunset::RequestGetWarpCount(IScript::Request& request) {
	CHECK_REFERENCES_NONE();
	return GetKernel().GetWarpCount();
}

TShared<RoutineGraph> BridgeSunset::RequestNewGraph(IScript::Request& request, int32_t startupWarp) {
	CHECK_REFERENCES_NONE();

	TShared<RoutineGraph> graph = TShared<RoutineGraph>::From(new RoutineGraph());
	graph->SetWarpIndex(GetKernel().GetCurrentWarpIndex());
	GetKernel().YieldCurrentWarp();

	return graph;
}

void BridgeSunset::RequestQueueGraphRoutine(IScript::Request& request, IScript::Delegate<RoutineGraph> graph, IScript::Delegate<WarpTiny> unit, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(graph);
	CHECK_DELEGATE(unit);

	uint32_t id = graph->Insert(GetKernel(), unit.Get(), CreateTaskScript(callback));
	request.DoLock();
	request << id;
	request.UnLock();
}

void BridgeSunset::RequestConnectGraphRoutine(IScript::Request& request, IScript::Delegate<RoutineGraph> graph, int32_t prev, int32_t next) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	graph->Next(prev, next);
}

void BridgeSunset::RequestExecuteGraph(IScript::Request& request, IScript::Delegate<RoutineGraph> graph) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	graph->Commit();
}
