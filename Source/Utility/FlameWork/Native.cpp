#include "Native.h"
#include <cmath>
#include <cstring>
#include <ctime>

using namespace PaintsNow;
using namespace PaintsNow::NsFlameWork;

/* test.c
int main(int argc, char* argv[]) {
	char* p = argv[1];
	int length = strlen(p) + 1;
	char* str = (char*)malloc(length);
	while (*p != '\0') {
		*p++ = toupper(*str++);
	}
	*p = '\0';
	response(argv, "result", str, length);
	free(str);
	return 0;
}
*/


/* test.lua
local code = [[ content of test.c above ]]
local native = FlameWork.CompileNativeCode(code)
if (native) then
	local ret = FlameWork.ExecuteNative("main", table.pack("let's go here."))
	if (ret) then
		print("Converted " .. ret.result)
	end
end
*/

template <class T>
IScript::Request::Ref MakeFunction(T t) {
	return IScript::Request::Ref(reinterpret_cast<size_t>(reinterpret_cast<void*>(t)));
}

IScript::Request::Ref MakeDoubleFunction1(double(*t)(double x)) {
	return MakeFunction(t);
}

IScript::Request::Ref MakeDoubleFunction2(double(*t)(double x, double y)) {
	return MakeFunction(t);
}

struct Bridge {
	std::map<String, String> outputs;
};

void WriteResponse(char* argv[], const char* tag, const char* value, size_t valueLength) {
	Bridge* bridge = reinterpret_cast<Bridge*>(argv[0]);
	if (bridge != nullptr) {
		bridge->outputs[tag].assign(value, value + valueLength);
	}
}

Native::Native(IScript& nativeScript) {
	nativeRequest = nativeScript.NewRequest();

	IScript::Request& request = *nativeRequest;
	request << global
		<< key("response") << MakeFunction(WriteResponse)
		<< key("malloc") << MakeFunction(malloc)
		<< key("free") << MakeFunction(free)
		<< key("clock") << MakeFunction(clock)
		<< key("memcpy") << MakeFunction(memcpy)
		<< key("strlen") << MakeFunction(strlen)
		<< key("strcpy") << MakeFunction(strcpy)
		<< key("strcmp") << MakeFunction(strcmp)
		<< key("strncmp") << MakeFunction(strncmp)
		<< key("memset") << MakeFunction(memset)
		<< key("memcmp") << MakeFunction(memcmp)
		<< key("memmove") << MakeFunction(memmove)
		<< key("rand") << MakeFunction(rand)
		<< key("srand") << MakeFunction(srand)
		<< key("qsort") << MakeFunction(qsort)
		<< key("sprintf") << MakeFunction(sprintf)
		<< key("sscanf") << MakeFunction(sscanf)
		<< key("qsort") << MakeFunction(qsort)
		<< key("bsearch") << MakeFunction(bsearch)
		<< key("fmod") << MakeDoubleFunction2(fmod)
		<< key("ceil") << MakeDoubleFunction1(ceil)
		<< key("floor") << MakeDoubleFunction1(floor)
		<< key("sinh") << MakeDoubleFunction1(sinh)
		<< key("cosh") << MakeDoubleFunction1(cosh)
		<< key("tanh") << MakeDoubleFunction1(tanh)
		<< key("sin") << MakeDoubleFunction1(sin)
		<< key("cos") << MakeDoubleFunction1(cos)
		<< key("tan") << MakeDoubleFunction1(tan)
		<< key("asin") << MakeDoubleFunction1(asin)
		<< key("acos") << MakeDoubleFunction1(acos)
		<< key("atan") << MakeDoubleFunction1(atan)
		<< key("atan2") << MakeDoubleFunction2(atan2)
		<< key("exp") << MakeDoubleFunction1(exp)
		<< key("log") << MakeDoubleFunction1(log)
		<< key("fabs") << MakeDoubleFunction1(fabs)
		<< key("sqrt") << MakeDoubleFunction1(sqrt)
		<< key("pow") << MakeDoubleFunction2(pow)
		<< key("toupper") << MakeFunction(toupper)
		<< key("isalpha") << MakeFunction(isalpha)
		<< key("isdigit") << MakeFunction(isdigit)
		<< key("tolower") << MakeFunction(tolower)
		<< key("isupper") << MakeFunction(isupper)
		<< key("isspace") << MakeFunction(isspace)
		<< key("isxdigit") << MakeFunction(isxdigit)
		<< key("isprint") << MakeFunction(isprint)
		<< key("iscntrl") << MakeFunction(iscntrl)
		<< endtable;
}


Native::~Native() {
	for (std::map<String, IScript::Request::Ref>::iterator p = routineRefs.begin(); p != routineRefs.end(); ++p) {
		nativeRequest->Dereference(p->second);
	}

	delete nativeRequest;
}

bool Native::Compile(const String& code) {
	IScript::Request::Ref ref = nativeRequest->Load(code, "main");
	compiled = ref.value != 0;
	nativeRequest->Dereference(ref);

	return compiled;
}


bool Native::Execute(IScript::Request& request, const String& name, const std::vector<String>& parameters) {
	if (compiled) {
		std::map<String, IScript::Request::Ref>::const_iterator p = routineRefs.find(name);
		IScript::Request::Ref ref;
		if (p == routineRefs.end()) {
			(*nativeRequest) << global >> key(name) >> ref << endtable;
			routineRefs[name] = ref;
		} else {
			ref = p->second;
		}

		Bridge bridge;
		Routine routine = reinterpret_cast<Routine>(ref.value);
		// Build param list
		std::vector<char*> argv(parameters.size() + 1);
		argv[0] = reinterpret_cast<char*>(&bridge);
		for (size_t i = 0; i < parameters.size(); i++) {
			argv[i + 1] = const_cast<char*>(parameters[i].data());
		}

		int retValue = routine((int)argv.size(), &argv[0]);

		// write returned data
		if (retValue == 0) {
			request.DoLock();
			request << beginarray;
			for (std::map<String, String>::const_iterator p = bridge.outputs.begin(); p != bridge.outputs.end(); ++p) {
				request << key(p->first) << p->second;
			}
			request << endarray;
			request.UnLock();
		}

		return true;
	} else {
		return false;
	}
}
