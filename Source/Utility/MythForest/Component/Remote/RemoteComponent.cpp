#include "RemoteComponent.h"

#include <utility>

using namespace PaintsNow;

static void ErrorHandler(IScript::Request& request, const String& err) {
	fprintf(stderr, "RemoteRoutine subscript error: %s\n", err.c_str());
}

RemoteRoutine::RemoteRoutine(IScript::RequestPool* p, IScript::Request::Ref r) : pool(p), ref(r) {}

RemoteRoutine::~RemoteRoutine() {
	Clear();
}

void RemoteRoutine::Clear() {
	if (ref) {
		IScript::Request& req = pool->GetScript().GetDefaultRequest();

		req.DoLock();
		req.Dereference(ref);
		ref.value = 0;
		req.UnLock();
	}
}

void RemoteRoutine::ScriptUninitialize(IScript::Request& request) {
	SharedTiny::ScriptUninitialize(request);
}

static void SysCall(IScript::Request& request, IScript::Delegate<RemoteRoutine> routine, IScript::Request::Arguments& args) {
	RemoteComponent* remoteComponent = static_cast<RemoteComponent*>(routine->pool);
	assert(remoteComponent != nullptr);

	remoteComponent->Call(request, routine.Get(), args);
}

static void SysCallAsync(IScript::Request& request, IScript::Request::Ref callback, IScript::Delegate<RemoteRoutine> routine, IScript::Request::Arguments& args) {
	RemoteComponent* remoteComponent = static_cast<RemoteComponent*>(routine->pool);
	assert(remoteComponent != nullptr);

	remoteComponent->CallAsync(request, callback, routine.Get(), args);
}

RemoteComponent::RemoteComponent(Engine& e) : RequestPool(*e.interfaces.script.NewScript(), e.GetKernel().GetWarpCount()), engine(e) {
	script.DoLock();
	IScript::Request& request = script.GetDefaultRequest();
	request << global << key("io") << nil << endtable;
	request << global << key("os") << nil << endtable;
	request << global << key("SysCall") << request.Adapt(Wrap(SysCall));
	request << global << key("SysCallAsync") << request.Adapt(Wrap(SysCallAsync));
	script.SetErrorHandler(Wrap(ErrorHandler));
	script.UnLock();
}

RemoteComponent::~RemoteComponent() {
	RequestPool::Clear();
	script.ReleaseDevice();	
}

TObject<IReflect>& RemoteComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {}

	return *this;
}

struct RoutineWrapper {
	RoutineWrapper(const TShared<RemoteRoutine>& r) : routine(std::move(r)) {}
	~RoutineWrapper() {}
	void operator () (IScript::Request& request, IScript::Request::Arguments& args) {
		// always use sync call
		RemoteComponent* remoteComponent = static_cast<RemoteComponent*>(routine->pool);
		assert(remoteComponent != nullptr);

		remoteComponent->Call(request, routine(), args);
	}

	TShared<RemoteRoutine> routine;
};

TShared<RemoteRoutine> RemoteComponent::Load(const String& code) {
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();
	IScript::Request::Ref ref = request.Load(code, "RemoteComponent");
	request.UnLock();

	if (ref.value != 0) {
		return TShared<RemoteRoutine>::From(new RemoteRoutine(this, ref));
	} else {
		return nullptr;
	}
}

static void CopyTable(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest);
static void CopyArray(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest);
static void CopyVariable(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest, IScript::Request::TYPE type) {
	switch (type) {
		case IScript::Request::NIL:
			request << nil;
			break;
		case IScript::Request::BOOLEAN:
		{
			bool value;
			fromRequest >> value;
			request << value;
			break;
		}
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
			CopyTable(flag, request, fromRequest);
			break;
		}
		case IScript::Request::ARRAY:
		{
			CopyArray(flag, request, fromRequest);
			break;
		}
		case IScript::Request::FUNCTION:
		{
			// convert to reference
			IScript::Request::Ref ref;
			fromRequest >> ref;
			// managed by remote routine
			TShared<RemoteRoutine> remoteRoutine = TShared<RemoteRoutine>::From(new RemoteRoutine(fromRequest.GetRequestPool(), ref));
			if (flag & RemoteComponent::REMOTECOMPONENT_TRANSPARENT) {
				// create wrapper
				RoutineWrapper routineWrapper(remoteRoutine);
				request << request.Adapt(WrapClosure(&routineWrapper, &RoutineWrapper::operator ()));
			} else {
				request << remoteRoutine;
			}

			break;
		}
		case IScript::Request::OBJECT:
		{
			IScript::BaseDelegate d;
			fromRequest >> d;
			IScript::Object* object = d.GetRaw();
			if (flag & RemoteComponent::REMOTECOMPONENT_TRANSPARENT) {
				RemoteRoutine* routine = object->QueryInterface(UniqueType<RemoteRoutine>());
				if (routine != nullptr) {
					if (&routine->pool->GetScript() == request.GetScript()) {
						request << routine->ref;
					} else {
						RoutineWrapper routineWrapper(routine);
						request << request.Adapt(WrapClosure(&routineWrapper, &RoutineWrapper::operator ()));
					}
				} else {
					request << object;
				}
			} else {
				request << object;
			}
			break;
		}
		case IScript::Request::ANY:
		{
			// omitted.
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
		fromRequest >> k;
		request << k;
		CopyVariable(flag, request, fromRequest, k.type);
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
		fromRequest >> k;
		request << k;
		CopyVariable(flag, request, fromRequest, k.type);
	}

	request << endtable;
	fromRequest >> endtable;
}

void RemoteComponent::Call(IScript::Request& fromRequest, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	if (remoteRoutine->pool == this && remoteRoutine->ref) {
		IScript::Request& toRequest = *AcquireSafe();
		toRequest.DoLock();
		fromRequest.DoLock();

		toRequest.Push();
		uint32_t flag = Flag().load(std::memory_order_relaxed);
		// read remaining parameters
		for (int i = 0; i < args.count; i++) {
			CopyVariable(flag, toRequest, fromRequest, fromRequest.GetCurrentType());
		}
		fromRequest.UnLock();
		toRequest.Call(sync, remoteRoutine->ref);

		fromRequest.DoLock();
		for (int k = 0; k < toRequest.GetCount(); k++) {
			CopyVariable(flag, fromRequest, toRequest, toRequest.GetCurrentType());
		}

		toRequest.Pop();
		fromRequest.UnLock();
		toRequest.UnLock();

		ReleaseSafe(&toRequest);
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

void RemoteComponent::Complete(IScript::RequestPool* returnPool, IScript::Request& toRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine) {
	toRequest.Call(sync, remoteRoutine->ref);

	IScript::Request& returnRequest = *returnPool->AcquireSafe();
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

	ReleaseSafe(&toRequest);
	returnPool->ReleaseSafe(&returnRequest);
}

void RemoteComponent::CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>&remoteRoutine, IScript::Request::Arguments& args) {
	if (remoteRoutine->pool == this && remoteRoutine->ref) {
		IScript::Request& toRequest = *AcquireSafe();
		fromRequest.DoLock();
		toRequest.DoLock();

		toRequest.Push();
		// read remaining parameters
		uint32_t flag = Flag().load(std::memory_order_relaxed);
		for (int i = 0; i < args.count; i++) {
			CopyVariable(flag, toRequest, fromRequest, fromRequest.GetCurrentType());
		}

		IScript::RequestPool* pool = fromRequest.GetRequestPool();
		assert(&pool->GetScript() == fromRequest.GetScript());
		fromRequest.UnLock();

		engine.bridgeSunset.GetKernel().threadPool.Push(CreateTaskContextFree(Wrap(this, &RemoteComponent::Complete), pool, std::ref(toRequest), callback, remoteRoutine));
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

