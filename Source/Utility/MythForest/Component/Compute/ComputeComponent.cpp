#include "ComputeComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

static void ErrorHandler(IScript::Request& request, const String& err) {
	fprintf(stderr, "ComputeRoutine subscript error: %s\n", err.c_str());
}

ComputeRoutine::ComputeRoutine(IScript::RequestPool* p, IScript::Request::Ref r) : pool(p), ref(r) {
	pool->GetScript().SetErrorHandler(Wrap(ErrorHandler));
}

ComputeRoutine::~ComputeRoutine() {
	Clear();
}

void ComputeRoutine::Clear() {
	if (ref) {
		IScript::Request& req = pool->GetScript().GetDefaultRequest();

		req.DoLock();
		req.Dereference(ref);
		ref.value = 0;
		req.UnLock();
	}
}

void ComputeRoutine::ScriptUninitialize(IScript::Request& request) {
	SharedTiny::ScriptUninitialize(request);
}

static void SysCall(IScript::Request& request, IScript::Delegate<ComputeRoutine> routine, IScript::Request::Arguments& args) {
	ComputeComponent* computeComponent = static_cast<ComputeComponent*>(routine->pool);
	assert(computeComponent != nullptr);

	computeComponent->Call(request, routine.Get(), args);
}

static void SysCallAsync(IScript::Request& request, IScript::Request::Ref callback, IScript::Delegate<ComputeRoutine> routine, IScript::Request::Arguments& args) {
	ComputeComponent* computeComponent = static_cast<ComputeComponent*>(routine->pool);
	assert(computeComponent != nullptr);

	computeComponent->CallAsync(request, callback, routine.Get(), args);
}

ComputeComponent::ComputeComponent(Engine& e) : RequestPool(*e.interfaces.script.NewScript(), e.GetKernel().GetWarpCount()), engine(e) {
	script.DoLock();
	IScript::Request& request = script.GetDefaultRequest();
	request << global << key("io") << nil << endtable;
	request << global << key("os") << nil << endtable;
	request << global << key("SysCall") << request.Adapt(Wrap(SysCall));
	request << global << key("SysCallAsync") << request.Adapt(Wrap(SysCallAsync));
	script.UnLock();
}

ComputeComponent::~ComputeComponent() {
	RequestPool::Clear();
	script.ReleaseDevice();	
}

TObject<IReflect>& ComputeComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {}

	return *this;
}

struct RoutineWrapper {
	RoutineWrapper(TShared<ComputeRoutine>& r) : routine(r) {}
	~RoutineWrapper() {}
	void operator () (IScript::Request& request, IScript::Request::Arguments& args) {
		// always use sync call
		ComputeComponent* computeComponent = static_cast<ComputeComponent*>(routine->pool);
		assert(computeComponent != nullptr);

		computeComponent->Call(request, routine(), args);
	}

	TShared<ComputeRoutine> routine;
};

TShared<ComputeRoutine> ComputeComponent::Load(const String& code) {
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();
	IScript::Request::Ref ref = request.Load(code, "ComputeComponent");
	request.UnLock();

	return TShared<ComputeRoutine>::From(new ComputeRoutine(this, ref));
}

static void CopyTable(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest);
static void CopyArray(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest);
static void CopyVariable(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest, IScript::Request::TYPE type) {
	switch (type) {
		case IScript::Request::NIL:
			request << nil;
			break;
		case IScript::Request::NUMBER:
		{
			double value;
			fromRequest >> value;
			request << value;
			break;
		}
		case IScript::Request::INTEGER:
		{
			int64_t value;
			fromRequest >> value;
			request << value;
			break;
		}
		case IScript::Request::STRING:
		{
			String value;
			fromRequest >> value;
			request << value;
			break;
		}
		case IScript::Request::TABLE:
		{
			request << begintable;
			CopyTable(flag, request, fromRequest);
			request << endtable;
			break;
		}
		case IScript::Request::ARRAY:
		{
			request << beginarray;
			CopyArray(flag, request, fromRequest);
			request << endarray;
			break;
		}
		case IScript::Request::FUNCTION:
		{
			// convert to reference
			IScript::Request::Ref ref;
			fromRequest >> ref;
			// managed by compute routine
			TShared<ComputeRoutine> computeRoutine = TShared<ComputeRoutine>::From(new ComputeRoutine(fromRequest.GetRequestPool(), ref));
			if (flag & ComputeComponent::COMPUTECOMPONENT_TRANSPARENT) {
				// create wrapper
				RoutineWrapper routineWrapper(computeRoutine);
				request << request.Adapt(WrapClosure(&routineWrapper, &RoutineWrapper::operator ()));
			} else {
				request << computeRoutine;
			}

			break;
		}
		case IScript::Request::OBJECT:
		{
			IScript::BaseDelegate d;
			fromRequest >> d;
			request << d.GetRaw();
			break;
		}
	}
}

static void CopyArray(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest) {
	IScript::Request::ArrayStart ts;
	fromRequest >> ts;
	request << beginarray;
	for (size_t j = 0; j < ts.count; j++) {
		CopyVariable(flag, request, fromRequest, fromRequest.GetCurrentType());
	}

	std::vector<IScript::Request::Key> keys = fromRequest.Enumerate();
	for (size_t i = 0; i < keys.size(); i++) {
		const IScript::Request::Key& k = keys[i];
		// NIL, NUMBER, INTEGER, STRING, TABLE, FUNCTION, OBJECT
		request >> key(k.GetKey());
		CopyVariable(flag, request, fromRequest, k.GetType());
	}

	request << endarray;
	fromRequest >> endarray;
}

static void CopyTable(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest) {
	IScript::Request::TableStart ts;
	fromRequest >> ts;
	request << begintable;
	for (size_t j = 0; j < ts.count; j++) {
		CopyVariable(flag, request, fromRequest, fromRequest.GetCurrentType());
	}

	std::vector<IScript::Request::Key> keys = fromRequest.Enumerate();
	for (size_t i = 0; i < keys.size(); i++) {
		const IScript::Request::Key& k = keys[i];
		// NIL, NUMBER, INTEGER, STRING, TABLE, FUNCTION, OBJECT
		request >> key(k.GetKey());
		CopyVariable(flag, request, fromRequest, k.GetType());
	}

	request << endtable;
	fromRequest >> endtable;
}

void ComputeComponent::Call(IScript::Request& fromRequest, TShared<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args) {
	if (computeRoutine->pool == this && computeRoutine->ref) {
		IScript::Request& toRequest = *AllocateRequest();
		toRequest.DoLock();
		fromRequest.DoLock();

		toRequest.Push();
		uint32_t flag = Flag().load(std::memory_order_relaxed);
		// read remaining parameters
		for (int i = 0; i < args.count; i++) {
			CopyVariable(flag, toRequest, fromRequest, fromRequest.GetCurrentType());
		}
		fromRequest.UnLock();
		toRequest.Call(sync, computeRoutine->ref);

		fromRequest.DoLock();
		for (int k = 0; k < toRequest.GetCount(); k++) {
			CopyVariable(flag, fromRequest, toRequest, toRequest.GetCurrentType());
		}

		toRequest.Pop();
		fromRequest.UnLock();
		toRequest.UnLock();

		FreeRequest(&toRequest);
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

void ComputeComponent::Complete(IScript::RequestPool* returnPool, IScript::Request& toRequest, IScript::Request::Ref callback, TShared<ComputeRoutine> computeRoutine) {
	toRequest.Call(sync, computeRoutine->ref);

	IScript::Request& returnRequest = *returnPool->AllocateRequest();
	returnRequest.DoLock();
	returnRequest.Push();

	uint32_t flag = Flag().load(std::memory_order_relaxed);
	for (int i = 0; i < toRequest.GetCount(); i++) {
		CopyVariable(flag, returnRequest, toRequest, toRequest.GetCurrentType());
	}

	toRequest.Pop();
	toRequest.UnLock();

	returnRequest.Call(sync, callback);
	returnRequest.Dereference(callback);
	returnRequest.Pop();
	returnRequest.UnLock();

	FreeRequest(&toRequest);
	returnPool->FreeRequest(&returnRequest);
}

void ComputeComponent::CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, TShared<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args) {
	if (computeRoutine->pool == this && computeRoutine->ref) {
		IScript::Request& toRequest = *AllocateRequest();
		fromRequest.DoLock();
		toRequest.DoLock();

		toRequest.Push();
		// read remaining parameters
		uint32_t flag = Flag().load(std::memory_order_relaxed);
		for (int i = 0; i < args.count; i++) {
			CopyVariable(flag, toRequest, fromRequest, fromRequest.GetCurrentType());
		}
		fromRequest.UnLock();
		assert(&fromRequest.GetRequestPool()->GetScript() == fromRequest.GetScript());

		engine.bridgeSunset.GetKernel().threadPool.Push(CreateTaskContextFree(Wrap(this, &ComputeComponent::Complete), fromRequest.GetRequestPool(), std::ref(toRequest), callback, computeRoutine));
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

