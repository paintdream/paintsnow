#include "ComputeComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ComputeRoutine::ComputeRoutine(IScript& s, IScript::Request::Ref r) : script(s), ref(r) {}
ComputeRoutine::~ComputeRoutine() {}

void ComputeRoutine::ScriptUninitialize(IScript::Request& request) {
	if (ref) {
		IScript::Request& req = script.GetDefaultRequest();

		req.DoLock();
		req.Dereference(ref);
		req.UnLock();
	}

	SharedTiny::ScriptUninitialize(request);
}

ComputeComponent::ComputeComponent(Engine& e, IScript* s, IScript& hostScript) : engine(e), script(s) {
	s->DoLock();
	mainRequest = s->NewRequest();

	IScript::Request& request = *mainRequest;
	request << global << key("io") << nil << endtable;
	request << global << key("os") << nil << endtable;
	request << global << key("SysCall") << request.Adapt(Wrap(this, &ComputeComponent::RequestSysCall)) << endtable;
	s->UnLock();

	hostScript.DoLock();
	hostRequest = hostScript.NewRequest();
	hostScript.UnLock();
}

ComputeComponent::~ComputeComponent() {
	script->DoLock();
	mainRequest->ReleaseObject();
	script->UnLock();
	script->ReleaseDevice();	

	IScript* hostScript = hostRequest->GetScript();
	hostScript->DoLock();
	hostRequest->ReleaseObject();
	hostScript->UnLock();
}

TObject<IReflect>& ComputeComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestSysCall)[ScriptMethod = "SysCall"];
	}

	return *this;
}

TShared<ComputeRoutine> ComputeComponent::Load(const String& code) {
	IScript::Request& request = *mainRequest;
	request.DoLock();
	IScript::Request::Ref ref = request.Load(code, "ComputeComponent");
	request.UnLock();

	return TShared<ComputeRoutine>::From(new ComputeRoutine(*request.GetScript(), ref));
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
			TShared<ComputeRoutine> computeRoutine = TShared<ComputeRoutine>::From(new ComputeRoutine(*fromRequest.GetScript(), ref));
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

template <bool lockOrder>
void TunnelCall(IScript::Request& toRequest, IScript::Request& fromRequest, TShared<ComputeRoutine> computeRoutine) {
	if (computeRoutine->ref) {
		if (lockOrder) {
			toRequest.DoLock();
			fromRequest.DoLock();
		} else {
			fromRequest.DoLock();
			toRequest.DoLock();
		}

		toRequest.Push();
		// read remaining parameters
		CopyTable(toRequest, fromRequest);

		if (lockOrder) {
			fromRequest.UnLock();
		}

		assert(toRequest.GetScript() == &computeRoutine->script);
		toRequest.Call(sync, computeRoutine->ref);

		if (lockOrder) {
			fromRequest.DoLock();
		}

		if (toRequest.GetCount() != 0) {
			// read remaining parameters
			CopyTable(fromRequest, toRequest);
		}

		toRequest.Pop();

		if (lockOrder) {
			fromRequest.UnLock();
			toRequest.UnLock();
		} else {
			toRequest.UnLock();
			fromRequest.UnLock();
		}
	}
}

void ComputeComponent::Call(IScript::Request& fromRequest, TShared<ComputeRoutine> computeRoutine) {
	TunnelCall<true>(*mainRequest, fromRequest, computeRoutine);
}

void ComputeComponent::Cleanup() {
	assert(false); // not available now.
}

void ComputeComponent::RequestSysCall(IScript::Request& request, IScript::Delegate<ComputeRoutine> computeRoutine, IScript::Request::PlaceHolder ph) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(computeRoutine);

	TunnelCall<false>(*hostRequest, request, computeRoutine.Get());
}