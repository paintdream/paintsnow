#include "ComputeComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ComputeRoutine::ComputeRoutine(IScript::RequestPool* p, IScript::Request::Ref r) : pool(p), ref(r) {}
ComputeRoutine::~ComputeRoutine() {}

void ComputeRoutine::ScriptUninitialize(IScript::Request& request) {
	if (ref) {
		IScript::Request& req = pool->GetScript().GetDefaultRequest();

		req.DoLock();
		req.Dereference(ref);
		req.UnLock();
	}

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
	script.ReleaseDevice();	
}

TObject<IReflect>& ComputeComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {}

	return *this;
}

TShared<ComputeRoutine> ComputeComponent::Load(const String& code) {
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();
	IScript::Request::Ref ref = request.Load(code, "ComputeComponent");
	request.UnLock();

	return TShared<ComputeRoutine>::From(new ComputeRoutine(this, ref));
}

static void CopyTable(IScript::Request& request, IScript::Request& fromRequest);
static void CopyArray(IScript::Request& request, IScript::Request& fromRequest);
static void CopyVariable(IScript::Request& request, IScript::Request& fromRequest, IScript::Request::TYPE type) {
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
			CopyTable(request, fromRequest);
			request << endtable;
			break;
		}
		case IScript::Request::ARRAY:
		{
			request << beginarray;
			CopyArray(request, fromRequest);
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
			request << computeRoutine;
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

static void CopyArray(IScript::Request& request, IScript::Request& fromRequest) {
	IScript::Request::ArrayStart ts;
	fromRequest >> ts;
	request << beginarray;
	for (size_t j = 0; j < ts.count; j++) {
		CopyVariable(request, fromRequest, fromRequest.GetCurrentType());
	}

	std::vector<IScript::Request::Key> keys = fromRequest.Enumerate();
	for (size_t i = 0; i < keys.size(); i++) {
		const IScript::Request::Key& k = keys[i];
		// NIL, NUMBER, INTEGER, STRING, TABLE, FUNCTION, OBJECT
		request >> key(k.GetKey());
		CopyVariable(request, fromRequest, k.GetType());
	}

	request << endarray;
	fromRequest >> endarray;
}

static void CopyTable(IScript::Request& request, IScript::Request& fromRequest) {
	IScript::Request::TableStart ts;
	fromRequest >> ts;
	request << begintable;
	for (size_t j = 0; j < ts.count; j++) {
		CopyVariable(request, fromRequest, fromRequest.GetCurrentType());
	}

	std::vector<IScript::Request::Key> keys = fromRequest.Enumerate();
	for (size_t i = 0; i < keys.size(); i++) {
		const IScript::Request::Key& k = keys[i];
		// NIL, NUMBER, INTEGER, STRING, TABLE, FUNCTION, OBJECT
		request >> key(k.GetKey());
		CopyVariable(request, fromRequest, k.GetType());
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
		// read remaining parameters
		for (int i = 0; i < args.count; i++) {
			CopyVariable(toRequest, fromRequest, fromRequest.GetCurrentType());
		}
		fromRequest.UnLock();
		toRequest.Call(sync, computeRoutine->ref);

		fromRequest.DoLock();
		for (int k = 0; k < toRequest.GetCount(); k++) {
			CopyVariable(fromRequest, toRequest, toRequest.GetCurrentType());
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

	for (int i = 0; i < toRequest.GetCount(); i++) {
		CopyVariable(returnRequest, toRequest, toRequest.GetCurrentType());
	}

	toRequest.Pop();

	returnRequest.Call(sync, callback);
	returnRequest.Dereference(callback);
	returnRequest.Pop();
	returnRequest.UnLock();
	toRequest.UnLock();

	FreeRequest(&toRequest);
	returnPool->FreeRequest(&returnRequest);
}

void ComputeComponent::CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, TShared<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args) {
	if (computeRoutine->pool == this && computeRoutine->ref) {
		IScript::Request& toRequest = *AllocateRequest();
		toRequest.DoLock();
		fromRequest.DoLock();

		toRequest.Push();
		// read remaining parameters
		for (int i = 0; i < args.count; i++) {
			CopyVariable(toRequest, fromRequest, fromRequest.GetCurrentType());
		}
		fromRequest.UnLock();

		engine.bridgeSunset.GetKernel().threadPool.Push(CreateTaskContextFree(Wrap(this, &ComputeComponent::Complete), fromRequest.GetRequestPool(), std::ref(toRequest), callback, computeRoutine));
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

