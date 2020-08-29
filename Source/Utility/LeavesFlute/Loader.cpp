#include "Loader.h"
// #include <vld.h>
#include "../LeavesFlute/Platform.h"

using namespace PaintsNow;

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_LZMA_BUILTIN
IArchive* Create7ZArchive(IStreamBase& streamBase, size_t length) {
	return new ZArchive7Z(streamBase, length);
}
#endif

class ZFrameDummy : public IFrame {
public:
	ZFrameDummy() {}
	virtual void SetCallback(Callback* callback) {}
	virtual const Int2& GetWindowSize() const {
		static Int2 size;
		return size;
	}

	virtual void SetWindowSize(const Int2& size) {}
	virtual void SetWindowTitle(const String& title) {}

	virtual void OnMouse(const EventMouse& mouse) {}
	virtual void OnKeyboard(const EventKeyboard& keyboard) {}
	virtual void OnRender() {}
	virtual void OnWindowSize(const Int2& newSize) {}
	virtual void EnterMainLoop() {}
	virtual void ExitMainLoop() {}
	virtual void ShowCursor(CURSOR cursor) {}
	virtual void WarpCursor(const Int2& position) {}
	virtual bool IsRendering() const {
		return true;
	}
};

class ZRenderDummy : public IRender {
public:
	virtual std::vector<String> EnumerateDevices() { return std::vector<String>(); }
	virtual Device* CreateDevice(const String& description) override { return nullptr; }
	virtual Int2 GetDeviceResolution(Device* device) override { return Int2(640, 480); }
	virtual void SetDeviceResolution(Device* device, const Int2& resolution) override {}
	virtual void NextDeviceFrame(Device* device) override {}
	virtual void DeleteDevice(Device* device) override {}

	// Queue
	virtual Device* GetQueueDevice(Queue* queue) override { return nullptr; }
	virtual Queue* CreateQueue(Device* device, uint32_t flag) override {
		static Queue q;
		return &q; // Make asserts happy
	}

	virtual bool SupportParallelPresent(Device* device) override { return false; }
	virtual void PresentQueues(Queue** queue, uint32_t count, PresentOption option) override {}
	virtual void DeleteQueue(Queue* queue) override {}
	virtual void FlushQueue(Queue* queue) override {}

	// Resource
	virtual Resource* CreateResource(Device* device, Resource::Type resourceType) override {
		static Resource r;
		return &r;
	}
	virtual void AcquireResource(Queue* queue, Resource* resource) override {}
	virtual void ReleaseResource(Queue* queue, Resource* resource) override {}
	virtual void UploadResource(Queue* queue, Resource* resource, Resource::Description* description) override {}
	virtual void RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) override {}
	virtual void CompleteDownloadResource(Queue* queue, Resource* resource) override {}
	virtual void ExecuteResource(Queue* queue, Resource* resource) override {}
	virtual void SwapResource(Queue* queue, Resource* lhs, Resource* rhs) override {}
	virtual void DeleteResource(Queue* queue, Resource* resource) override {}
	virtual void SetResourceNotation(Resource* lhs, const String& note) {}
};


void Loader::SetFactory(TWrapper<IDevice*>& ptr, const String& key, const std::map<String, CmdLine::Option>& factoryMap) {
	const std::map<String, CmdLine::Option>::const_iterator p = factoryMap.find(key);
	if (p != factoryMap.end()) {
		const String& impl = p->second.name;
		const std::list<Config::Entry>& entries = config.GetEntry(key);
		for (std::list<Config::Entry>::const_iterator q = entries.begin(); q != entries.end(); ++q) {
			// printf("Scanning [%s] = %s (%s)\n", key.c_str(), (*q).name.c_str(), param.c_str());
			if ((ptr == (*q).factoryBase && impl.empty()) || (*q).name == impl) {
				ptr = (*q).factoryBase;
				break;
			}
		}
	}

	if (!ptr) {
		fprintf(stderr, "Couldn't find factory for %s\n", key.c_str());
		exit(-1);
	}
}

template <class T>
struct DelayedReference {
	DelayedReference(T*& p) : ptr(p) {}

	operator T& () {
		return *ptr;
	}

	T*& ptr;
};

template <class T>
struct DelayedValue {
	DelayedValue(T& p) : value(p) {}

	operator T& () {
		return value;
	}

	T& value;
};

void Loader::Load(const CmdLine& cmdLine) {
	// Load necessary modules
	const std::list<CmdLine::Option>& modules = cmdLine.GetModuleList();

	printf("LeavesFlute %s\nPaintDream (paintdream@paintdream.com) (C) 2014-2020\nBased on PaintsNow [https://github.com/paintdream/paintsnow]\n", PAINTSNOW_VERSION_MINOR);

	bool nogui = true;
	bool isVulkan = true;
	bool enableModuleLog = false;
	const std::map<String, CmdLine::Option>& configMap = cmdLine.GetConfigMap();
	std::string entry = "Entry";
	uint32_t threadCount = 4;
	
	uint32_t warpCount = 0; // let mythforest decide
	const uint32_t maxWarpCount = 1 << WarpTiny::WARP_BITS;
	const uint32_t maxThreadCount = maxWarpCount >> 1;

	for (std::map<String, CmdLine::Option>::const_iterator p = configMap.begin(); p != configMap.end(); ++p) {
		if ((*p).first == "Graphic") {
			nogui = (*p).second.name != "true";
		} else if ((*p).first == "Log") {
			enableModuleLog = (*p).second.name == "true";
		} else if ((*p).first == "Entry") {
			entry = (*p).second.name;
		} else if ((*p).first == "Warp") {
			// According to Unit::WARP_INDEX
			warpCount = Math::Min(maxWarpCount, (uint32_t)safe_cast<uint32_t>(atoi((*p).second.name.c_str())));
		} else if ((*p).first == "Thread") {
			int32_t expectedThreadCount = (int32_t)safe_cast<int32_t>(atoi((*p).second.name.c_str()));
#ifdef _WIN32
			// full speed
			if (expectedThreadCount <= 0) {
				SYSTEM_INFO systemInfo;
				::GetSystemInfo(&systemInfo);
				expectedThreadCount = Math::Max((int32_t)threadCount, (int32_t)systemInfo.dwNumberOfProcessors - expectedThreadCount);
			}
#else
			if (expectedThreadCount == 0) {
				expectedThreadCount = threadCount;
			}
#endif
			threadCount = Math::Min(maxThreadCount, (uint32_t)expectedThreadCount);
		}
	}

	// warpCount = Math::Max(warpCount, threadCount);

	const std::map<String, CmdLine::Option>& factoryMap = cmdLine.GetFactoryMap();

	if (enableModuleLog) {
		for (std::map<String, CmdLine::Option>::const_iterator i = factoryMap.begin(); i != factoryMap.end(); ++i) {
			printf("Load config [%s] = %s Param: %s\n", (*i).first.c_str(), (*i).second.name.c_str(), (*i).second.param.c_str());
			assert(factoryMap.find(i->first) != factoryMap.end());
			// printf("Compare result: %d\n", strcmp(i->first.c_str(), "IRender"));
		}
	}

	// assert(factoryMap.find(String("IArchive")) != factoryMap.end());
	// assert(factoryMap.find(String("IRender")) != factoryMap.end());

	// Load default settings
	TWrapper<IThread*> threadFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_THREAD_PTHREAD
	threadFactory = WrapFactory(UniqueType<ZThreadPthread>());
	config.RegisterFactory("IThread", "ZThreadPthread", threadFactory);
#endif

	TWrapper<IFrame*> frameFactory = WrapFactory(UniqueType<ZFrameDummy>());
	config.RegisterFactory("IFrame", "ZFrameDummy", frameFactory);

	TWrapper<IRender*> renderFactory = WrapFactory(UniqueType<ZRenderDummy>());
	config.RegisterFactory("IRender", "ZRenderDummy", renderFactory);

#if (!defined(CMAKE_PAINTSNOW) || ADD_RENDER_VULKAN) && (!defined(_MSC_VER) || _MSC_VER > 1200)
	GLFWwindow* window = nullptr;
	if (!nogui) {
		renderFactory = WrapFactory(UniqueType<ZRenderVulkan>(), DelayedValue<GLFWwindow*>(window));
		config.RegisterFactory("IRender", "ZRenderVulkan", renderFactory);
	}
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_RENDER_OPENGL
	static TWrapper<IRender*> glFactory = WrapFactory(UniqueType<ZRenderOpenGL>());
	if (!nogui) {
		renderFactory = glFactory;
		config.RegisterFactory("IRender", "ZRenderOpenGL", renderFactory);
	}
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FRAME_GLFW
	if (!nogui) {
#if (!defined(CMAKE_PAINTSNOW) || ADD_RENDER_VULKAN) && (!defined(_MSC_VER) || _MSC_VER > 1200)
		GLFWwindow** ptr = &window;
#else
		GLFWwindow** ptr = nullptr;
#endif
		frameFactory = WrapFactory(UniqueType<ZFrameGLFW>(), DelayedValue<GLFWwindow**>(ptr), DelayedValue<bool>(isVulkan));
		config.RegisterFactory("IFrame", "ZFrameGLFW", frameFactory);
	}
#endif

	TWrapper<IArchive*, IStreamBase&, size_t> subArchiveCreator;

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_LZMA_BUILTIN
	subArchiveCreator = Create7ZArchive;
#endif

	TWrapper<IScript*> scriptFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_SCRIPT_LUA_BUILTIN
	scriptFactory = WrapFactory(UniqueType<ZScriptLua>(), DelayedReference<IThread>(thread));
	config.RegisterFactory("IScript", "ZScriptLua", scriptFactory);
#endif

	TWrapper<INetwork*> networkFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_LIBEVENT
	networkFactory = WrapFactory(UniqueType<ZNetworkLibEvent>(), DelayedReference<IThread>(thread));
	config.RegisterFactory("INetwork", "ZNetworkLibEvent", networkFactory);
#endif

	tunnelFactory = networkFactory;

	TWrapper<IImage*> imageFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_IMAGE_FREEIMAGE
	imageFactory = WrapFactory(UniqueType<ZImageFreeImage>());
	config.RegisterFactory("IImage", "ZImageFreeImage", imageFactory);
#endif

	TWrapper<IRandom*> randomFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_RANDOM_LIBNOISE_BUILTIN
	randomFactory = WrapFactory(UniqueType<ZRandomLibnoisePerlin>());
	config.RegisterFactory("IRandom", "ZRandomLibnoisePerlin", randomFactory);
#endif

	TWrapper<IFontBase*> fontFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_FONT_FREETYPE
	fontFactory = WrapFactory(UniqueType<ZFontFreetype>());
	config.RegisterFactory("IFontBase", "ZFontFreetype", fontFactory);
#endif

	TWrapper<IFilterBase*> audioFilterFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_LAME
	audioFilterFactory = WrapFactory(UniqueType<ZFilterLAME>());
	config.RegisterFactory("IFIlterBase::Audio", "ZDecoderLAME", audioFilterFactory);
#endif

	TWrapper<IDatabase*> databaseFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_DATABASE_SQLITE3_BUILTIN
	databaseFactory = WrapFactory(UniqueType<ZDatabaseSqlite>());
	config.RegisterFactory("IDatabase", "ZDatabaseSqlite", databaseFactory);
#endif

	TWrapper<ITimer*> timerFactory;
#if (defined(_WIN32) || defined(WIN32)) && ((!defined(CMAKE_PAINTSNOW) || ADD_TIMER_TIMERQUEUE_BUILTIN))
	timerFactory = WrapFactory(UniqueType<ZWinTimerQueue>());
	config.RegisterFactory("ITimer", "ZWinTimerQueue", timerFactory);
#endif

#if !(defined(_WIN32) || defined(WIN32)) && (!defined(CMAKE_PAINTSNOW) || ADD_TIMER_POSIX_BUILTIN)
	timerFactory = WrapFactory(UniqueType<ZPosixTimer>());
	config.RegisterFactory("ITimer", "ZPosixTimer", timerFactory);
#endif

	TWrapper<IAudio*> audioFactory;
#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_OPENAL
	audioFactory = WrapFactory(UniqueType<ZAudioOpenAL>());
	config.RegisterFactory("IAudio", "ZAudioOpenAL", audioFactory);
#endif

	TWrapper<IArchive*> archiveFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_ARCHIVE_DIRENT_BUILTIN
	archiveFactory = WrapFactory(UniqueType<ZArchiveDirent>());
	config.RegisterFactory("IArchive", "ZArchiveDirent", archiveFactory);
#endif

	TWrapper<IFilterBase*> assetFilterFactory;

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_POD_BUILTIN
	assetFilterFactory = WrapFactory(UniqueType<ZFilterPod>());
	config.RegisterFactory("IFilterBase::Asset", "ZFilterPod", assetFilterFactory);
#else
	assetFilterFactory = WrapFactory(UniqueType<NoFilter>());
	config.RegisterFactory("IFilterBase::Asset", "NoFilter", assetFilterFactory);
#endif

	SetFactory((TWrapper<IDevice*>&)renderFactory, "IRender", factoryMap);
	SetFactory((TWrapper<IDevice*>&)frameFactory, "IFrame", factoryMap);
	SetFactory((TWrapper<IDevice*>&)threadFactory, "IThread", factoryMap);
	SetFactory((TWrapper<IDevice*>&)audioFactory, "IAudio", factoryMap);
	SetFactory((TWrapper<IDevice*>&)archiveFactory, "IArchive", factoryMap);
	SetFactory((TWrapper<IDevice*>&)scriptFactory, "IScript", factoryMap);
	SetFactory((TWrapper<IDevice*>&)networkFactory, "INetwork", factoryMap);
	SetFactory((TWrapper<IDevice*>&)randomFactory, "IRandom", factoryMap);
	SetFactory((TWrapper<IDevice*>&)timerFactory, "ITimer", factoryMap);
	SetFactory((TWrapper<IDevice*>&)imageFactory, "IImage", factoryMap);
	SetFactory((TWrapper<IDevice*>&)assetFilterFactory, "IFilterBase::Asset", factoryMap);
	SetFactory((TWrapper<IDevice*>&)fontFactory, "IFontBase", factoryMap);
	SetFactory((TWrapper<IDevice*>&)audioFilterFactory, "IFilterBase::Audio", factoryMap);
	SetFactory((TWrapper<IDevice*>&)databaseFactory, "IDatabase", factoryMap);

	if (!nogui) {
		// Must have render factory in GUI mode.
		assert(renderFactory);

#if !defined(CMAKE_PAINTSNOW) || (ADD_RENDER_OPENGL && ADD_FRAME_GLFW)
		// initialize for GLFW
		if (renderFactory == glFactory) {
			isVulkan = false; // fallback to opengl
		}
#endif
	}

	thread = threadFactory(); // precreate thread
	mainThread = thread->OpenCurrentThread();
	{
		frame = frameFactory();

		IScript* script = scriptFactory();
		IRender* render = renderFactory();
		ITimer* timer = timerFactory();
		IImage* image = imageFactory();
		INetwork* network = networkFactory();
		ITunnel* tunnel = tunnelFactory();
		IAudio* audio = audioFactory();
		IArchive* archive = archiveFactory();
		IRandom* random = randomFactory();
		IDatabase* database = databaseFactory();
		IFilterBase* assetFilter = assetFilterFactory();
		IFilterBase* audioFilter = audioFilterFactory();
		IFontBase* font = fontFactory();

		{
			Interfaces interfaces(*archive, *audio, *database, *assetFilter, *audioFilter, *font, *frame, *image, *network, *random, *render, *script, *thread, *timer, *tunnel);
			LeavesFlute leavesFlute(nogui, interfaces, subArchiveCreator, threadCount, warpCount);
			this->leavesFlute = &leavesFlute;

			std::vector<String> paramList;
			std::map<String, CmdLine::Option>::const_iterator scriptParam = factoryMap.find("IScript");
			if (scriptParam != factoryMap.end()) {
				paramList = Split(scriptParam->second.param);
			}

			leavesFlute.Execute(entry, paramList);

			std::map<String, CmdLine::Option>::const_iterator quit = configMap.find("Quit");
			if (quit == configMap.end() || (*quit).second.name != "true") {
				if (nogui) {
					leavesFlute.EnterStdinLoop();
				} else {
					leavesFlute.EnterMainLoop();
				}
			}
		}

		font->ReleaseDevice();
		assetFilter->ReleaseDevice();
		audioFilter->ReleaseDevice();
		database->ReleaseDevice();
		random->ReleaseDevice();
		tunnel->ReleaseDevice();
		image->ReleaseDevice();
		network->ReleaseDevice();
		script->ReleaseDevice();
		timer->ReleaseDevice();
		render->ReleaseDevice();
		audio->ReleaseDevice();
		archive->ReleaseDevice();

		frame->ReleaseDevice();
	}

	thread->DeleteThread(mainThread);
	delete thread;
}

#ifdef _WIN32
#include <Windows.h>
#ifndef _DEBUG
extern "C" void _except_handler4_common() {}
#endif
#endif

Loader::Loader() : leavesFlute(nullptr) {
#ifdef _WIN32
	::CoInitialize(nullptr);
#endif
}

Loader::~Loader() {
#ifdef _WIN32
	::CoUninitialize();
#endif
}

