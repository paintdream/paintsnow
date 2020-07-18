#include "ScriptComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

class Reflector : public IReflect {
public:
	Reflector(unordered_map<String, size_t>& m) : mapEventNameToID(m), IReflect(false, false, false, true) {}

	virtual void Enum(size_t value, Unique id, const char* name, const MetaChainBase* meta) override {
		mapEventNameToID[name] = value;
	}

private:
	unordered_map<String, size_t>& mapEventNameToID;
};

ScriptComponentModule::ScriptComponentModule(Engine& engine) : BaseClass(engine) {
	// register enums
	Reflector reflector(mapEventNameToID);
	(*engine.GetComponentModuleFromName("EventComponent"))(reflector);
}

ScriptComponentModule::~ScriptComponentModule() {}

TObject<IReflect>& ScriptComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetHandler)[ScriptMethod = "SetHandler"];
	}

	return *this;
}

TShared<ScriptComponent> ScriptComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();
	TShared<ScriptComponent> scriptComponent = TShared<ScriptComponent>::From(allocator->New());

	return scriptComponent;
}

void ScriptComponentModule::RequestSetHandler(IScript::Request& request, IScript::Delegate<ScriptComponent> scriptComponent, const String& event, IScript::Request::Ref handler) {
	if (handler) {
		CHECK_REFERENCES_WITH_TYPE(handler, IScript::Request::FUNCTION);
	}

	unordered_map<String, size_t>::iterator it = mapEventNameToID.find(event);
	if (it == mapEventNameToID.end()) {
		if (handler) {
			request.DoLock();
			request.Dereference(handler);
			request.UnLock();
		}

		request.Error(String("Unable to find event: ") + event);
	} else {
		scriptComponent->SetHandler(request, static_cast<Event::EVENT_ID>((*it).second), handler);
	}
}
