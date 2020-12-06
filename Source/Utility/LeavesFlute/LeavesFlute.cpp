#include "LeavesFlute.h"
#include <ctime>
#include "../../Core/Driver/Filter/Pod/ZFilterPod.h"
#include "../../General/Driver/Filter/Json/ZFilterJson.h"
#include "../../General/Driver/Filter/LZMA/ZFilterLZMA.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#endif

using namespace PaintsNow;

class ScanModules : public IReflect {
public:
	ScanModules() : IReflect(true, false) {}
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (!s.IsBasicObject() && s.QueryInterface(UniqueType<IScript::Library>()) != nullptr) {
			modules.emplace_back(static_cast<IScript::Library*>(&s));
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	std::vector<IScript::Library*> modules;
};

LeavesFlute::LeavesFlute(bool ng, Interfaces& inters, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const String& defMount, uint32_t threadCount, uint32_t warpCount) :
					ISyncObject(inters.thread),
					interfaces(inters),
					bridgeSunset(inters.thread, inters.script, threadCount, warpCount),
					echoLegend(inters.thread, inters.network, bridgeSunset),
					snowyStream(inters, bridgeSunset, subArchiveCreator, defMount, Wrap(this, &LeavesFlute::OnConsoleOutput)),
					mythForest(inters, snowyStream, bridgeSunset),
					heartVioliner(inters.thread, inters.timer, bridgeSunset),
					remembery(inters.thread, inters.archive, inters.database, bridgeSunset),
					galaxyWeaver(inters.thread, inters.tunnel, bridgeSunset, snowyStream, mythForest),
					consoleThread(nullptr)
{
	ScanModules scanModules;
	(*this)(scanModules);
	std::swap(modules, scanModules.modules);
	Initialize();

	interfaces.frame.SetCallback(this);
	IScript::Request& request = interfaces.script.GetDefaultRequest();
	request.DoLock();
	Require(request); // register callbacks
	request.UnLock();
}

/*
void LeavesFlute::Reset(bool reload) {
	resetting = true;
	IScript::Request& request = interfaces.script.GetDefaultRequest();
	request.DoLock();
	if (listenConsole) {
		request.Dereference(listenConsole);
	}
	ScriptUninitialize(request);
	request.UnLock();
	interfaces.script.Reset();

	if (reload) {
		IScript::Request& request = interfaces.script.GetDefaultRequest();

		request.DoLock();
		ScriptInitialize(request);
		request.UnLock();
	}

	resetting = false;
}
*/

LeavesFlute::~LeavesFlute() {
	IScript::Request& request = interfaces.script.GetDefaultRequest();
	request.DoLock();
	if (listenConsole) {
		request.Dereference(listenConsole);
	}
	request.UnLock();
	interfaces.script.Reset();

	Uninitialize();
}

void LeavesFlute::EnterStdinLoop() {
	printf("Init Standard Input Environment ...\n");
	ConsoleProc(nullptr, 0);
}

void LeavesFlute::EnterMainLoop() {
	BeginConsole();
	interfaces.frame.EnterMainLoop();

#ifdef _WIN32
	::FreeConsole();
	// interfaces.thread.Wait(consoleThread);
#endif
	EndConsole();
}

void LeavesFlute::BeginConsole() {
	assert(consoleThread == nullptr);
	consoleThread = interfaces.thread.NewThread(Wrap(this, &LeavesFlute::ConsoleProc), 0);
}

void LeavesFlute::EndConsole() {
	assert(consoleThread != nullptr);
	threadApi.DeleteThread(consoleThread);
	consoleThread = nullptr;
}

#include <iostream>
#ifdef WIN32
#include <Windows.h>

bool ReadConsoleSafe(WCHAR buf[], DWORD size, DWORD* read, HANDLE h) {
	// may crash or memory leak when user close the window on win7
	// but why ?
	__try {
		if (::ReadConsoleW(h, buf, size - 1, read, nullptr)) {
			return true;
		}
	} __except (EXCEPTION_ACCESS_VIOLATION) {
		printf("Unexpected access violation\n");
	}

	return false;
}

#else
#include <sys/poll.h>
#endif

bool LeavesFlute::ProcessCommand(const String& command) {
	if (consoleThread == nullptr && (command == "quit" || command == "exit")) {
		return false;
	} else {
		IScript::Request& request = interfaces.script.GetDefaultRequest();
		if (!listenConsole) {
			request.DoLock();
			IScript::Request::Ref code = request.Load(command, "Console");
			request.UnLock();

			if (code) {
				bridgeSunset.Dispatch(CreateTaskScriptOnce(code));
			}
		} else {
			bridgeSunset.Dispatch(CreateTaskScript(listenConsole, command));
		}

		return true;
	}
}

bool LeavesFlute::ConsoleProc(IThread::Thread* thread, size_t index) {
#ifndef WIN32
	pollfd cinfd[1];
	cinfd[0].fd = fileno(stdin);
	cinfd[0].events = POLLIN;
#else
	HANDLE h = ::GetStdHandle(STD_INPUT_HANDLE);
#endif
	printf("=> ");
	fflush(stdout);
	while (true) {
#ifndef WIN32
		int ret = poll(cinfd, 1, 1000);
		if (ret > 0 && cinfd[0].revents == POLLIN) {
			String command;
			getline(std::cin, command);
			if (command[command.size() - 1] == '\n') {
				command = command.substr(0, command.size() - 1);
			}

			// Process Command
			if (!command.empty()) {
				IScript::Request& request = interfaces.script.GetDefaultRequest();
				// remove uncessary spaces
				if (!ProcessCommand(String(command.c_str(), command.length()))) {
					break;
				}
			}
			printf("=> ");
			fflush(stdout);
		} else if (ret < 0){
			break;
		}
#else
		DWORD ret = ::WaitForSingleObject(h, INFINITE);

		if (ret == WAIT_OBJECT_0) {
			const size_t CMD_SIZE = 1024;
			static WCHAR buf[CMD_SIZE];
			DWORD read;
			if (ReadConsoleSafe(buf, CMD_SIZE, &read, h)) {
				String unicode((const char*)buf, sizeof(buf[0]) * read);
				// Process Command
				if (buf[0] != '\0') {
					String command = SystemToUtf8(unicode);
					// remove tail spaces and returns
					int32_t end = (int32_t)command.size() - 1;
					while (end >= 0) {
						char ch = command[end];
						if (ch == '\r' || ch == '\n' || ch == '\t' || ch == ' ') {
							end--;
						} else {
							break;
						}
					}

					if (!ProcessCommand(command.substr(0, end + 1)))
						break;
				}
				printf("=> ");
			} else {
				break;
			}
		} else if (ret == WAIT_FAILED) {
			break;
		}
#endif
	}

	return false;
}

void LeavesFlute::OnInitialize(void* param) {}

template <class T>
void WriteObjectAttrib(IScript::Request& request, T& container) {
	for (typename T::const_iterator p = container.begin(); p != container.end(); ++p) {
		request << key((*p).first) << (*p).second;
	}

	container.clear();
}

void LeavesFlute::RequestSetScreenSize(IScript::Request& request, Int2& size) {
	interfaces.frame.SetWindowSize(size);
}

Int2 LeavesFlute::RequestGetScreenSize(IScript::Request& request) {
	return interfaces.frame.GetWindowSize();
}

void LeavesFlute::RequestWarpCursor(IScript::Request& request, Int2 position) {
	interfaces.frame.WarpCursor(position);
}

void LeavesFlute::RequestShowCursor(IScript::Request& request, const String& type) {
	IFrame::CURSOR cursor = IFrame::ARROW;
	if (type == "None") {
		cursor = IFrame::NONE;
	} else if (type == "Arrow") {
		cursor = IFrame::ARROW;
	} else if (type == "Cross") {
		cursor = IFrame::CROSS;
	} else if (type == "Wait") {
		cursor = IFrame::WAIT;
	}

	interfaces.frame.ShowCursor(cursor);
}

void LeavesFlute::RequestSetAppTitle(IScript::Request& request, const String& title) {
	DoLock();
	newAppTitle = title;
	UnLock();
}

void LeavesFlute::RequestForward(IScript::Request& request, IScript::Request::Ref ref) {
	CHECK_REFERENCES_WITH_TYPE(ref, IScript::Request::FUNCTION);
	request.DoLock();
	request.Call(deferred, ref);
	request.Dereference(ref);
	request.UnLock();
}

bool LeavesFlute::IsRendering() const {
	return interfaces.frame.IsRendering();
}

Interfaces& LeavesFlute::GetInterfaces() const {
	return interfaces;
}

Kernel& LeavesFlute::GetKernel() {
	return bridgeSunset.GetKernel();
}

void LeavesFlute::Require(IScript::Request& request) {
	// disable extra libs
	// request << global << key("package") << nil << endtable;
	request << global << key("io") << nil << endtable;
	// request << global << key("debug") << nil << endtable;
	request << global << key("os") << nil << endtable;

#ifdef _DEBUG
	request << global << key("DEBUG") << true << endtable;
#else
	request << global << key("DEBUG") << false << endtable;
#endif

#ifdef _WIN32
	request << global << key("DLL") << String(".dll") << endtable;
#else
	request << global << key("DLL") << String(".so") << endtable;
#endif

#if defined(_WIN32)
	char ver[256];
	sprintf(ver, "MSVC %d", _MSC_VER);
	request << global << key("Compiler") << String(ver) << endtable;
#elif defined(__VERSION__)
	request << global << key("Compiler") << String(__VERSION__) << endtable;
#endif

	request.GetScript()->SetErrorHandler(Wrap(this, &LeavesFlute::RequestPrint));

	// takeover the default print call and redirect it to our console.
	request << global << key("System");
	Library::Require(request);
	request << endtable;
}

void LeavesFlute::RequestExit(IScript::Request& request) {
	if (consoleThread != nullptr) {
#ifndef WIN32
		raise(SIGINT);
#else
		FreeConsole();
		::Sleep(100);
#endif
	}

	interfaces.frame.ExitMainLoop();
}

void LeavesFlute::RequestListenConsole(IScript::Request& request, IScript::Request::Ref ref) {
	if (ref) {
		CHECK_REFERENCES_WITH_TYPE(ref, IScript::Request::FUNCTION);
	}

	if (listenConsole) {
		request.DoLock();
		request.Dereference(listenConsole);
		request.UnLock();
	}

	listenConsole = ref;
}

void LeavesFlute::RequestPrint(IScript::Request& request, const String& text) {
	Print(text);
}

void LeavesFlute::OnConsoleOutput(const String& text) {
	DoLock();
	Print(text);
	UnLock();
}

void LeavesFlute::Print(const String& str) {
	// convert utf8 to system encoding
	const String& text = Utf8ToSystem(str);
#if defined(_WIN32) || defined(WIN32)
	// wprintf(L"%s\n", text.c_str());
	static HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD ws;
	WriteConsoleW(handle, text.c_str(), (DWORD)wcslen((const WCHAR*)text.c_str()), &ws, nullptr);
	printf("\n");
#else
	printf("%s\n", text.c_str());
#endif
}

void LeavesFlute::OnWindowSize(const IFrame::EventSize& size) {
	interfaces.render.SetDeviceResolution(snowyStream.GetRenderDevice(), size.size);
	mythForest.OnSize(size.size);
}

void LeavesFlute::OnMouse(const IFrame::EventMouse& mouse) {
	mythForest.OnMouse(mouse);
}

void LeavesFlute::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	mythForest.OnKeyboard(keyboard);
}

void LeavesFlute::OnRender() {
	bool titleChanged = false;
	DoLock();
	if (!(appTitle == newAppTitle)) {
		appTitle = newAppTitle;
		titleChanged = true;
	}
	UnLock();

	if (titleChanged) {
		interfaces.frame.SetWindowTitle(appTitle);
	}

	Int2 size = interfaces.frame.GetWindowSize();
	for (size_t i = 0; i < modules.size(); i++) {
		modules[i]->TickDevice(interfaces.render);
	}

	interfaces.render.NextDeviceFrame(snowyStream.GetRenderDevice());
}

class ExpandParamsScriptTask : public WarpTiny, public TaskRepeat {
public:
	ExpandParamsScriptTask(Kernel& k, const String& p, const std::vector<String>& params, Interfaces& inters) : kernel(k), path(p), value(params), interfaces(inters) {}

	bool LoadScriptText(const String& path, String& text) {
		uint64_t length = 0;
		bool ret = false;
		IStreamBase* stream = interfaces.archive.Open(path + "." + interfaces.script.GetFileExt(), false, length);
		if (stream == nullptr) {
			stream = interfaces.archive.Open(path, false, length);
		}

		if (stream != nullptr) {
			if (length != 0) {
				// read string
				size_t len = safe_cast<size_t>(length);
				text.resize(len);
				if (stream->Read(const_cast<char*>(text.data()), len)) {
					ret = true;
				}
			} else {
				ret = true;
			}

			stream->Destroy();
		}

		return ret;
	}

	void Abort(void* context) override {
		ReleaseObject();
	}

	void Execute(void* context) override {
		BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
		IScript::Request& request = *bridgeSunset.AcquireSafe();
		String text;
		if (LoadScriptText(path, text)) {
			request.DoLock();
			IScript::Request::Ref ref = request.Load(text, path);
			if (ref) {
				request.Push();
				for (size_t i = 0; i < value.size(); i++) {
					request << value[i];
				}
				request.Call(sync, ref);
				request.Dereference(ref);
				printf("=> ");
				request.Pop();
			}

			request.UnLock();
		}

		bridgeSunset.ReleaseSafe(&request);
		ReleaseObject();
	}

	Kernel& kernel;
	String path;
	Interfaces& interfaces;
	std::vector<String> value;
};

void LeavesFlute::Execute(const String& path, const std::vector<String>& params) {
	Kernel& kernel = bridgeSunset.GetKernel();
	ExpandParamsScriptTask* task = new ExpandParamsScriptTask(kernel, path, params, interfaces);
	uint32_t warpIndex = task->GetWarpIndex();
	ThreadPool& threadPool = kernel.GetThreadPool();
	kernel.QueueRoutine(task, task);
}

struct InspectCustomStructure : public IReflect {
	InspectCustomStructure(IScript::Request& r, IReflectObject& obj) : request(r), IReflect(true, false) {
		request << begintable;
		String name, count;
		InspectCustomStructure::FilterType(obj.GetUnique()->GetName(), name, count);

		request << key("Type") << name;
		request << key("Optional") << false;
		request << key("Pointer") << false;
		request << key("List") << false;
		request << key("Fields") << begintable;
		obj(*this);
		request << endtable;
		request << endtable;
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		singleton Unique typedBaseType = UniqueType<IScript::MetaVariable::TypedBase>::Get();
		for (const MetaChainBase* t = meta; t != nullptr; t = t->GetNext()) {
			const MetaNodeBase* node = t->GetNode();
			if (!node->IsBasicObject() && node->GetUnique() == typedBaseType) {
				IScript::MetaVariable::TypedBase* entry = const_cast<IScript::MetaVariable::TypedBase*>(static_cast<const IScript::MetaVariable::TypedBase*>(node));
				request << key(entry->name.empty() ? name : entry->name);
				InspectCustomStructure::ProcessMember(request, typeID, s.IsIterator(), false);
			}
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	static bool FilterType(const String& name, String& ret, String& count) {
		// parse name
		if (name == UniqueType<String>::Get()->GetName() || name == UniqueType<Bytes>::Get()->GetName()) {
			ret = "string";
			return true;
		} else if (name.find("std::vector") == 0 || name.find("std::pair") == 0 || name.find("std::list") == 0) {
			ret = "vector";
			return false;
		} else {
			ret = "";
			bool inword = false;
			for (size_t i = 0; i < name.length(); i++) {
				char ch = name[i];
				if (isalnum(ch) || ch == '_' || ch == ':') {
					if (!inword) ret = "";
					ret += ch;
					inword = true;
				} else {
					inword = false;
				}
			}
		}

		static const String typePrefix = "TType";
		String::size_type pos = name.find(typePrefix);
		String::size_type end = pos;
		if (pos != String::npos) {
			pos += typePrefix.length();
			end += typePrefix.length();
			while (end < name.length() && isdigit(name[end])) end++;
		}

		count = "";
		if (end != String::npos) {
			count = name.substr(pos, end - pos);
		}

		return true;
	}

	static void ProcessMember(IScript::Request& request, Unique type, bool isIterator, bool optional) {
		// filter TType
		String typeName = type->GetName();

		bool isDelegate = typeName.find("PaintsNow::BaseDelegate") != 0 || typeName.find("PaintsNow::Delegate<") != 0;
		String name, count;
		bool subfield = InspectCustomStructure::FilterType(typeName, name, count);

		if (type->IsCreatable()) {
			IReflectObject* obj = type->Create();
			if (obj != nullptr) {
				InspectCustomStructure(request, *obj);
				obj->Destroy();
			} else {
				request << begintable;
				request << key("Pointer") << false;
				request << key("Optional") << false;
				request << key("List") << false;
				request << key("Fields") << beginarray << endarray;
				request << key("Type") << String("ErrorType");
				request << endtable;
				assert(false);
			}
		} else {
			request << begintable;
			request << key("Optional") << (isDelegate && optional);
			request << key("Pointer") << count.empty();
			request << key("List") << isIterator;
			request << key("Fields") << beginarray;

			if (subfield) {
				int c = atoi(count.c_str());
				for (int i = 0; i < c; i++) {
					request << name;
				}
			}

			request << endarray;
			request << key("Type") << String(name + count);
			request << endtable;
		}
	}

	IScript::Request& request;
};

class IInspectPrimitive {
public:
	virtual void Write(IScript::Request& request, const void* p) = 0;
	virtual void Read(IScript::Request& request, void* p) = 0;
};

template <class T>
class InspectPrimitiveImpl : public IInspectPrimitive {
public:
	void Write(IScript::Request& request, const void* p) override {
		const T* t = reinterpret_cast<const T*>(p);
		request << *t;
	}

	void Read(IScript::Request& request, void* p) override {
		T& t = *reinterpret_cast<T*>(p);
		request >> t;
	}
};

#define REGISTER_PRIMITIVE(f) \
	{ static InspectPrimitiveImpl<f> prim_##f; \
	mapInspectors[UniqueType<f>::Get()] = &prim_##f; }

struct InspectPrimitives {
	InspectPrimitives() {
		REGISTER_PRIMITIVE(String);
		REGISTER_PRIMITIVE(bool);
		REGISTER_PRIMITIVE(size_t);
		REGISTER_PRIMITIVE(int8_t);
		REGISTER_PRIMITIVE(uint8_t);
		REGISTER_PRIMITIVE(int16_t);
		REGISTER_PRIMITIVE(uint16_t);
		REGISTER_PRIMITIVE(int32_t);
		REGISTER_PRIMITIVE(uint32_t);

		REGISTER_PRIMITIVE(int64_t);
		REGISTER_PRIMITIVE(uint64_t);
		REGISTER_PRIMITIVE(float);
		REGISTER_PRIMITIVE(double);

		REGISTER_PRIMITIVE(Int2);
		REGISTER_PRIMITIVE(Int3);
		REGISTER_PRIMITIVE(Int4);
		REGISTER_PRIMITIVE(Float2);
		REGISTER_PRIMITIVE(Float2Pair);
		REGISTER_PRIMITIVE(Float3);
		REGISTER_PRIMITIVE(Float3Pair);
		REGISTER_PRIMITIVE(Float4);
		REGISTER_PRIMITIVE(Float4Pair);
		REGISTER_PRIMITIVE(Double2);
		REGISTER_PRIMITIVE(Double2Pair);
		REGISTER_PRIMITIVE(Double3);
		REGISTER_PRIMITIVE(Double3Pair);
		REGISTER_PRIMITIVE(Double4);
		REGISTER_PRIMITIVE(Double4Pair);
		REGISTER_PRIMITIVE(MatrixFloat3x3);
		REGISTER_PRIMITIVE(MatrixFloat4x4);
	}

	std::map<Unique, IInspectPrimitive*> mapInspectors;
};

struct InspectProcs : public IReflect {
	InspectProcs(IScript::Request& r, IReflectObject& obj) : request(r), IReflect(false, true, true, true) {
		request << begintable;
		obj(*this);
		request << endtable;
	}

	void Enum(size_t value, Unique id, const char* name, const MetaChainBase* meta) override {
		request << key(name) << safe_cast<int32_t>(value);
	}

	void WriteReflectObject(IReflectObjectComplex* p) {
		if (p != nullptr) {
			if (p->QueryInterface(UniqueType<IScript::Object>()) != nullptr) {
				request << static_cast<IScript::Object*>(p);
			} else {
				(*p)(*this);
			}
		} else {
			request << nil;
		}
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		static InspectPrimitives inspectPrimitives;
		if (s.IsBasicObject()) {
			if (typeID != refTypeID) {
				request << key(name);
				WriteReflectObject(*reinterpret_cast<IReflectObjectComplex**>(ptr));
			} else {
				std::map<Unique, IInspectPrimitive*>::const_iterator p = inspectPrimitives.mapInspectors.find(typeID);
				if (p != inspectPrimitives.mapInspectors.end()) {
					request << key(name);
					p->second->Write(request, ptr);
				}
			}
		} else if (s.IsIterator()) {
			IIterator& it = static_cast<IIterator&>(s);
			if (it.IsElementBasicObject()) {
				Unique unique = it.GetElementUnique();
				if (unique != refTypeID) {
					request << key(name) << beginarray;
					while (it.Next()) {
						WriteReflectObject(*reinterpret_cast<IReflectObjectComplex**>(it.Get()));
					}
					request << endarray;
				} else {
					std::map<Unique, IInspectPrimitive*>::const_iterator p = inspectPrimitives.mapInspectors.find(it.GetElementUnique());
					if (p != inspectPrimitives.mapInspectors.end()) {
						request << key(name) << beginarray;

						while (it.Next()) {
							p->second->Write(request, it.Get());
						}

						request << endarray;
					}
				}
			} else {
				request << key(name) << beginarray;
				while (it.Next()) {
					IReflectObjectComplex& sub = *reinterpret_cast<IReflectObjectComplex*>(it.Get());
					WriteReflectObject(&sub);
				}
				request << endarray;
			}
		} else {
			request << key(name);
			WriteReflectObject(static_cast<IReflectObjectComplex*>(&s));
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {
		// convert params ...
		while (meta != nullptr) {
			const MetaNodeBase* node = meta->GetNode();
			singleton Unique ScriptMethodUnique = UniqueType<IScript::MetaMethod::TypedBase>::Get();
			if (node->GetUnique() == ScriptMethodUnique) {
				const IScript::MetaMethod::TypedBase* entry = static_cast<const IScript::MetaMethod::TypedBase*>(node);
				request << key(entry->name.empty() ? name : entry->name) << begintable << key("Params") << beginarray;
				for (size_t i = 1; i < params.size(); i++) {
					InspectCustomStructure::ProcessMember(request, params[i].decayType, false, true);
				}

				request << endarray;
				request << key("Returns");
				request << begintable;
				InspectCustomStructure::ProcessMember(request, retValue.decayType, false, true);
				request << endtable;
				request << endtable;
				break;
			}

			meta = meta->GetNext();
		}
	}

	IScript::Request& request;
};

void LeavesFlute::RequestInspect(IScript::Request& request, IScript::BaseDelegate d) {
	CHECK_REFERENCES_NONE();

	if (d.GetRaw() != nullptr) {
		bridgeSunset.GetKernel().YieldCurrentWarp();

		request.DoLock();
		InspectProcs procs(request, *d.GetRaw());
		request.UnLock();
	}
}

#ifdef _MSC_VER
#include <Windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

void LeavesFlute::RequestSearchMemory(IScript::Request& request, const String& memory, size_t start, size_t end, uint32_t alignment, uint32_t maxResult) {
	bridgeSunset.GetKernel().YieldCurrentWarp();
#ifdef _MSC_VER
	SYSTEM_INFO systemInfo;
	::GetSystemInfo(&systemInfo);
	assert((start & (alignment - 1)) == 0);
	size_t pageSize = systemInfo.dwPageSize;
	uint32_t count = 0;
	std::vector<size_t> addresses;

	for (size_t addr = start; addr < end; addr += pageSize) {
		MEMORY_BASIC_INFORMATION mbi;
		if (::VirtualQuery((LPVOID)addr, &mbi, sizeof(mbi)) != 0) {
			size_t regionEnd = Math::Min(end, (size_t)mbi.BaseAddress + (size_t)mbi.RegionSize);
			if ((mbi.State & MEM_COMMIT) && !(mbi.Protect & (PAGE_WRITECOMBINE | PAGE_NOCACHE | PAGE_GUARD))) {
				for (size_t p = addr; p < regionEnd - memory.size(); p += alignment) {
					if (memcmp((const void*)p, memory.data(), memory.size()) == 0) {
						addresses.emplace_back(p);

						if (maxResult != 0 && ++count >= maxResult) {
							addr = end;
							break;
						}
					}
				}
			}

			addr = (regionEnd + pageSize - 1) & ~(pageSize - 1);
		}
	}

	request.DoLock();
	request << addresses;
	request.UnLock();

#endif
}

TObject<IReflect>& LeavesFlute::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		// dependency order
		ReflectProperty(bridgeSunset)[ScriptLibrary = "BridgeSunset"];
		ReflectProperty(remembery)[ScriptLibrary = "Remembery"];
		ReflectProperty(echoLegend)[ScriptLibrary = "EchoLegend"];
		ReflectProperty(heartVioliner)[ScriptLibrary = "HeartVioliner"];
		ReflectProperty(snowyStream)[ScriptLibrary = "SnowyStream"];
		ReflectProperty(mythForest)[ScriptLibrary = "MythForest"];
		ReflectProperty(galaxyWeaver)[ScriptLibrary = "GalaxyWeaver"];
	}

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestInspect)[ScriptMethod = "Inspect"];
		ReflectMethod(RequestExit)[ScriptMethod = "Exit"];
		ReflectMethod(RequestPrint)[ScriptMethod = "Print"];
		ReflectMethod(RequestForward)[ScriptMethod = "Forward"];
		ReflectMethod(RequestSetAppTitle)[ScriptMethod = "SetAppTitle"];
		ReflectMethod(RequestShowCursor)[ScriptMethod = "ShowCursor"];
		ReflectMethod(RequestWarpCursor)[ScriptMethod = "WrapCursor"];
		ReflectMethod(RequestSetScreenSize)[ScriptMethod = "SetScreenSize"];
		ReflectMethod(RequestGetScreenSize)[ScriptMethod = "GetScreenSize"];
		ReflectMethod(RequestListenConsole)[ScriptMethod = "ListenConsole"];
		ReflectMethod(RequestSearchMemory)[ScriptMethod = "SearchMemory"];
	}

	return *this;
}
