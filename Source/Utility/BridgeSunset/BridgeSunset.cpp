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
		ReflectMethod(RequestQueueGraphRoutine)[ScriptMethodLocked = "QueueGraphRoutine"];
		ReflectMethod(RequestConnectGraphRoutine)[ScriptMethodLocked = "ConnectGraphRoutine"];
		ReflectMethod(RequestExecuteGraph)[ScriptMethod = "ExecuteGraph"];
		ReflectMethod(RequestQueueRoutine)[ScriptMethodLocked = "QueueRoutine"]; // Auto-locked
		ReflectMethod(RequestGetWarpCount)[ScriptMethodLocked = "GetWarpCount"];
		ReflectMethod(RequestSetWarpIndex)[ScriptMethodLocked = "UpdateWarpIndex"];
		ReflectMethod(RequestGetWarpIndex)[ScriptMethodLocked = "GetWarpIndex"];
		ReflectMethod(RequestPin)[ScriptMethodLocked = "Pin"];
		ReflectMethod(RequestUnpin)[ScriptMethodLocked = "Unpin"];
		ReflectMethod(RequestClone)[ScriptMethod = "Clone"];
	}

	return *this;
}

Kernel& BridgeSunset::GetKernel() {
	return kernel;
}

void BridgeSunset::Dispatch(ITask* task) {
	threadPool.Push(task);
}

void BridgeSunset::ContinueScriptDispatcher(IScript::Request& request, IHost* host, size_t paramCount, const TWrapper<void, IScript::Request&>& continuer) {
	// check if current warp is yielded
	static thread_local uint32_t stackWarpIndex = ~(uint32_t)0;
	uint32_t warpIndex = kernel.GetCurrentWarpIndex();
	if (warpIndex != ~(uint32_t)0) {
		stackWarpIndex = warpIndex;
		continuer(request);
		stackWarpIndex = warpIndex;
	} else {
		uint32_t saveStackWarpIndex = stackWarpIndex;
		if (saveStackWarpIndex != ~(uint32_t)0) {
			request.UnLock();
			kernel.WaitWarp(stackWarpIndex);
			request.DoLock();
		}

		continuer(request);
		stackWarpIndex = saveStackWarpIndex;
		kernel.YieldCurrentWarp();
	}
}

void BridgeSunset::RequestQueueRoutine(IScript::Request& request, IScript::Delegate<WarpTiny> unit, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(unit);

	if (GetKernel().GetCurrentWarpIndex() != unit->GetWarpIndex()) {
		GetKernel().QueueRoutine(unit.Get(), CreateTaskScriptOnce(callback));
	} else {
		// Already locked!
		// request.DoLock();
		request.Push();
		request.Call(callback);
		request.Dereference(callback);
		request.Pop();
		// request.UnLock();
	}
}

uint32_t BridgeSunset::RequestGetWarpCount(IScript::Request& request) {
	CHECK_REFERENCES_NONE();
	return GetKernel().GetWarpCount();
}

TShared<TaskGraph> BridgeSunset::RequestNewGraph(IScript::Request& request, int32_t startupWarp) {
	CHECK_REFERENCES_NONE();
	return TShared<TaskGraph>::From(new TaskGraph(kernel));
}

size_t BridgeSunset::RequestQueueGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, IScript::Delegate<WarpTiny> unit, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(graph);
	CHECK_DELEGATE(unit);

	return graph->Insert(unit.Get(), CreateTaskScriptOnce(callback));
}

void BridgeSunset::RequestConnectGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, int32_t prev, int32_t next) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	graph->Next(prev, next);
}

void BridgeSunset::RequestExecuteGraph(IScript::Request& request, IScript::Delegate<TaskGraph> graph) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	graph->Commit();
}

void BridgeSunset::RequestSetWarpIndex(IScript::Request& request, IScript::Delegate<WarpTiny> source, uint32_t index) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	source->SetWarpIndex(index);
}

uint32_t BridgeSunset::RequestGetWarpIndex(IScript::Request& request, IScript::Delegate<WarpTiny> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	return source->GetWarpIndex();
}

TShared<SharedTiny> BridgeSunset::RequestClone(IScript::Request& request, IScript::Delegate<SharedTiny> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	return static_cast<SharedTiny*>(source->Clone());
}

void BridgeSunset::RequestPin(IScript::Request& request, IScript::Delegate<WarpTiny> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	source->Flag().fetch_or(Tiny::TINY_PINNED);
}

void BridgeSunset::RequestUnpin(IScript::Request& request, IScript::Delegate<WarpTiny> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	source->Flag().fetch_and(~Tiny::TINY_PINNED);
}

