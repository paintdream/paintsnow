#include "ZScriptTCC.h"

using namespace PaintsNow;

// only one script tcc is allowed to present at the same time within one module.

bool ZScriptTCC::instanceMutex = false;


ZScriptTCC::ZScriptTCC(IThread& threadApi) : IScript(threadApi) {
	assert(!instanceMutex);
	instanceMutex = true;
	defaultRequest = NewRequest("");
}

ZScriptTCC::~ZScriptTCC() {
	assert(instanceMutex);
	delete defaultRequest;
	instanceMutex = false;
}

void ZScriptTCC::Reset() {}

IScript::Request& ZScriptTCC::Request::CleanupIndex() {
	return *this;
}

IScript::Request& ZScriptTCC::GetDefaultRequest() {
	// TODO: insert return statement here
	return *defaultRequest;
}

IScript::Request* ZScriptTCC::NewRequest(const String& entry) {
	return new Request(this);
}

TObject<IReflect>& ZScriptTCC::Request::operator () (IReflect& reflect) {
	return *this;
}


ZScriptTCC::Request::Request(ZScriptTCC* h) {
	state = tcc_new();
}

ZScriptTCC::Request::~Request() {
	tcc_delete(state);
}

IScript* ZScriptTCC::Request::GetScript() {
	return host;
}

int ZScriptTCC::Request::GetCount() {
	return 0;
}

IScript::Request::TYPE ZScriptTCC::Request::GetCurrentType() {
	return NIL;
}

std::vector<IScript::Request::Key> ZScriptTCC::Request::Enumerate() {
	assert(false); // not implemented
	return std::vector<IScript::Request::Key>();
}

IScript::Request& ZScriptTCC::Request::operator << (const TableStart&) {
	assert(GetScript()->GetLockCount() != 0);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (TableStart& ts) {
	assert(GetScript()->GetLockCount() != 0);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const ArrayStart&) {
	assert(GetScript()->GetLockCount() != 0);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (ArrayStart& ts) {
	assert(GetScript()->GetLockCount() != 0);
	return *this;
}

IScript::Request& ZScriptTCC::Request::Push() {
	assert(GetScript()->GetLockCount() != 0);
	assert(false); // not implemented
	return *this;
}

IScript::Request& ZScriptTCC::Request::Pop() {
	assert(GetScript()->GetLockCount() != 0);
	assert(false); // not implemented
	return *this;
}

IScript::Request::Ref ZScriptTCC::Request::Load(const String& script, const String& pa) {
	assert(binary.empty() != script.empty());
	assert(GetScript()->GetLockCount() != 0);

	if (binary.empty() && tcc_compile_string(state, script.c_str())) {
		// TODO: add all predefined symbols if needed

		// allocate space
		size_t size = tcc_relocate(state, nullptr);
		binary.resize(size);
		tcc_relocate(state, const_cast<char*>(binary.data()));
	}

	return IScript::Request::Ref(binary.empty() ? 0 : reinterpret_cast<size_t>(tcc_get_symbol(state, pa.empty() ? "main" : pa.c_str())));
}


IScript::Request& ZScriptTCC::Request::operator << (const Nil&) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false); // not implemented

	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const Global&) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const Local&) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const Key& k) {
	assert(GetScript()->GetLockCount() != 0);
	key = k.GetKey();
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (const Key& k) {
	assert(GetScript()->GetLockCount() != 0);
	key = k.GetKey();

	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (double value) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (double& value) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const String& str) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (String& str) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const char* str) {
	assert(GetScript()->GetLockCount() != 0);
	assert(str != nullptr);
	assert(false);
	return *this << String(str);
}

IScript::Request& ZScriptTCC::Request::operator >> (const char*& str) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false); // Not allowed
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (bool b) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (bool& b) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const BaseDelegate& value) {
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (BaseDelegate& value) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const AutoWrapperBase& wrapper) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (int64_t u) {
	assert(GetScript()->GetLockCount() != 0);
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (int64_t& u) {	
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::MoveVariables(IScript::Request& target, size_t count) {
	assert(false);
	return *this;
}


IScript::Request& ZScriptTCC::Request::operator << (const TableEnd&) {
	assert(GetScript()->GetLockCount() != 0);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (const TableEnd&) {
	assert(GetScript()->GetLockCount() != 0);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const ArrayEnd&) {
	assert(GetScript()->GetLockCount() != 0);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (const ArrayEnd&) {
	assert(GetScript()->GetLockCount() != 0);
	return *this;
}

bool ZScriptTCC::Request::IsValid(const BaseDelegate& d) {
	return d.GetRaw() != nullptr;
}

IScript::Request& ZScriptTCC::Request::operator >> (Ref& ref) {
	assert(!binary.empty());
	if (!key.empty()) {
		ref.value = reinterpret_cast<size_t>(tcc_get_symbol(state, key.c_str()));
	}

	return *this;
}

IScript::Request& ZScriptTCC::Request::operator << (const Ref& ref) {
	assert(binary.empty());
	if (!key.empty() && ref) {
		tcc_add_symbol(state, key.c_str(), reinterpret_cast<void*>(ref.value));
	}

	return *this;
}

bool ZScriptTCC::Request::Call(const AutoWrapperBase& wrapper, const Request::Ref& g) {
	assert(false);
	return false;
}

IScript::Request& ZScriptTCC::Request::operator >> (Arguments& args) {
	assert(false);
	return *this;
}

IScript::Request& ZScriptTCC::Request::operator >> (const Skip& skip) {
	assert(false);
	return *this;
}

IScript::Request::Ref ZScriptTCC::Request::Reference(const Ref& d) {
	return IScript::Request::Ref();
}

IScript::Request::TYPE ZScriptTCC::Request::GetReferenceType(const Ref& d) {
	return IScript::Request::OBJECT;
}

void ZScriptTCC::Request::Dereference(Ref& ref) {
	// No need to dereference
}

const char* ZScriptTCC::GetFileExt() const {
	static const char* extc = "c";
	return extc;
}

IScript* ZScriptTCC::NewScript() const {
	return nullptr; // Not supported
}