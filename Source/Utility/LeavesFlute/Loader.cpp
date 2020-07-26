#include "Loader.h"
// #include <vld.h>
#include "../LeavesFlute/Platform.h"

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesFlute;
using namespace PaintsNow::NsLeavesFlute;

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
	virtual Device* CreateDevice(const String& description) override { return nullptr; }
	virtual Int2 GetDeviceResolution(Device* device) override { return Int2(640, 480); }
	virtual void SetDeviceResolution(Device* device, const Int2& resolution) override {}
	virtual void DeleteDevice(Device* device) override {}

	// Queue
	virtual Device* GetQueueDevice(Queue* queue) override { return nullptr; }
	virtual Queue* CreateQueue(Device* device, bool shared) override {
		static Queue q;
		return &q; // Make asserts happy
	}

	virtual bool SupportParallelPresent(Device* device) override { return false; }
	virtual void PresentQueues(Queue** queue, uint32_t count, PresentOption option) override {}
	virtual void DeleteQueue(Queue* queue) override {}
	virtual void YieldQueue(Queue* queue) override {}
	virtual void MergeQueue(Queue* target, Queue* src) override {}
	virtual bool IsQueueEmpty(Queue* queue) override {
		return true;
	}

	// Resource
	virtual Resource* CreateResource(Queue* queue, Resource::Type resourceType) override {
		static Resource r;
		return &r;
	}
	virtual void UploadResource(Queue* queue, Resource* resource, Resource::Description* description) override {}
	virtual void RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) override {}
	virtual void CompleteDownloadResource(Queue* queue, Resource* resource) override {}
	virtual void ExecuteResource(Queue* queue, Resource* resource) override {}
	virtual void SwapResource(Queue* queue, Resource* lhs, Resource* rhs) override {}
	virtual void DeleteResource(Queue* queue, Resource* resource) override {}
};


void Loader::SetFactory(const void*& ptr, String& param, const String& key, const std::map<String, CmdLine::Option>& factoryMap) {
	const std::map<String, CmdLine::Option>::const_iterator p = factoryMap.find(key);
	if (p != factoryMap.end()) {
		const String& impl = p->second.name;
		const std::list<Config::Entry>& entries = config.GetEntry(key);
		for (std::list<Config::Entry>::const_iterator q = entries.begin(); q != entries.end(); ++q) {
			// printf("Scanning [%s] = %s (%s)\n", key.c_str(), (*q).name.c_str(), param.c_str());
			if ((ptr == (*q).factoryBase && impl.empty()) || (*q).name == impl) {
				ptr = (*q).factoryBase;
				param = p->second.param;
				// printf("Factory[%s] = %s (%s)\n", key.c_str(), impl.c_str(), param.c_str());
				break;
			}
		}
	}

	if (ptr == nullptr) {
		fprintf(stderr, "Couldn't find factory for %s\n", key.c_str());
		exit(-1);
	}
}


void Loader::Load(const CmdLine& cmdLine) {
	// Load necessary modules
	const std::list<CmdLine::Option>& modules = cmdLine.GetModuleList();

	printf("LeavesFlute %s\nPaintDream (paintdream@paintdream.com) (C) 2014-2020\nBased on PaintsNow [https://github.com/paintdream/paintsnow]\n", PAINTSNOW_VERSION_MINOR);

	bool nogui = true;
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
			// According to NsMythForest::Unit::WARP_INDEX
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

	const TFactory<ZFrameDummy, IFrame> sdummyframe;
	frameFactory = &sdummyframe;
	config.RegisterFactory("IFrame", "ZFrameDummy", sdummyframe);

#if !defined(CMAKE_PAINTSNOW) || ADD_FRAME_FREEGLUT
	const TFactory<ZFrameFreeglut, IFrame> sframeFactory;
	if (!nogui) {
		frameFactory = &sframeFactory;
	}

	config.RegisterFactory("IFrame", "ZFrameFreeglut", sframeFactory);

	const TFactory<ZTimerFreeglut, ITimer> stimerFactoryDef;
	config.RegisterFactory("ITimer", "ZTimerFreeglut", stimerFactoryDef);
	config.RegisterFactory("ITimerForFrame", "ZTimerFreeglut", stimerFactoryDef);
	timerFactoryForFrame = &stimerFactoryDef;
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FRAME_DXUT
#ifdef CMAKE_PAINTSNOW
	const TFactory<ZFrameDXUT, IFrame> sframeFactoryDXUT;
	const TFactory<ZTimerDXUT, ITimer> stimerFactoryDXUT;
	config.RegisterFactory("IFrame", "ZFrameDXUT", sframeFactoryDXUT);
	config.RegisterFactory("ITimerForFrame", "ZTimerDXUT", stimerFactoryDXUT);
	config.RegisterFactory("ITimer", "ZTimerDXUT", stimerFactoryDXUT);
#endif
#endif
	const TFactory<ZRenderDummy, IRender> srenderFactoryDummy;
	renderFactory = &srenderFactoryDummy;
	config.RegisterFactory("IRender", "ZRenderDummy", srenderFactoryDummy);

#if !defined(CMAKE_PAINTSNOW) || ADD_RENDER_OPENGL
	const TFactory<ZRenderOpenGL, IRender> srenderFactory;
	if (!nogui) {
		renderFactory = &srenderFactory;
		config.RegisterFactory("IRender", "ZRenderOpenGL", srenderFactory);
	}
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_RENDER_DIRECTX
#ifdef CMAKE_PAINTSNOW
	if (!nogui) {
		const FactoryOpenGL<ZRenderDirectX9> srenderFactoryDX9(this);
		config.RegisterFactory("IRender", "ZRenderDirectX9", srenderFactoryDX9);
	}
#endif
#endif

	TWrapper<IArchive*, IStreamBase&, size_t> subArchiveCreator;

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_LZMA_BUILTIN
	subArchiveCreator = Create7ZArchive;
#endif

	if (!nogui) {
		// Must have render factory in GUI mode.
		assert(renderFactory != nullptr);
	}

#if !defined(CMAKE_PAINTSNOW) || ADD_SCRIPT_LUA53_BUILTIN
	const FactoryInitWithThread<ZScriptLua, IScript> sscriptFactory(this);
	scriptFactory = &sscriptFactory;
	config.RegisterFactory("IScript", "ZScriptLua", sscriptFactory);
#endif

	// Must have script factory.
	assert(scriptFactory != nullptr);

#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_LIBEVENT
	const FactoryInitWithThread<ZNetworkLibEvent, ITunnel> snetworkFactory(this);

	networkFactory = &snetworkFactory;
	config.RegisterFactory("INetwork", "ZNetworkLibEvent", *networkFactory);
#endif

	tunnelFactory = networkFactory;

	// Must have network factory.
	assert(networkFactory != nullptr);
	assert(tunnelFactory != nullptr);

#if !defined(CMAKE_PAINTSNOW) || ADD_IMAGE_FREEIMAGE
	static const TFactory<ZImageFreeImage, IImage> simageFactory;
	imageFactory = &simageFactory;
	config.RegisterFactory("IImage", "ZImageFreeImage", simageFactory);
#endif

	// Must have image factory.
	assert(imageFactory != nullptr);

#if !defined(CMAKE_PAINTSNOW) || ADD_RANDOM_LIBNOISE_BUILTIN
	static const TFactory<ZRandomLibnoisePerlin, IRandom> srandomFactory;
	randomFactory = &srandomFactory;
	config.RegisterFactory("IRandom", "ZRandomLibnoisePerlin", srandomFactory);
#endif

	// Must have random factory.
	assert(randomFactory != nullptr);

#if !defined(CMAKE_PAINTSNOW) || ADD_FONT_FREETYPE
	static const TFactory<ZFontFreetype, IFontBase> sfontFactory;
	fontFactory = &sfontFactory;
	config.RegisterFactory("IFontBase", "ZFontFreetype", sfontFactory);
#endif

	if (!nogui) {
		assert(fontFactory != nullptr);
	}


#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_LAME
	static const TFactory<ZFilterLAME, IFilterBase> sdecoderFactory;
	audioFilterFactory = &sdecoderFactory;
	config.RegisterFactory("IFIlterBase::Audio", "ZDecoderLAME", sdecoderFactory);
#endif

	assert(audioFilterFactory != nullptr);

#if !defined(CMAKE_PAINTSNOW) || ADD_DATABASE_SQLITE3_BUILTIN
	static const TFactory<ZDatabaseSqlite, IDatabase> sdatabaseFactory;
	databaseFactory = &sdatabaseFactory;
	config.RegisterFactory("IDatabase", "ZDatabaseSqlite", sdatabaseFactory);
#endif

	assert(databaseFactory != nullptr);

#if (defined(_WIN32) || defined(WIN32)) && (!defined(CMAKE_PAINTSNOW) || ADD_TIMER_TIMERQUEUE_BUILTIN)
	static const TFactory<ZWinTimerQueue, ITimer> stimerFactory;
	config.RegisterFactory("ITimer", "ZWinTimerQueue", stimerFactory);
	// if (timerFactory == nullptr)
	timerFactory = &stimerFactory;

	if (timerFactoryForFrame == nullptr) {
		timerFactoryForFrame = timerFactory;
	}
#endif

#if !(defined(_WIN32) || defined(WIN32)) && (!defined(CMAKE_PAINTSNOW) || ADD_TIMER_POSIX_BUILTIN)
	static const TFactory<ZPosixTimer, ITimer> pstimerFactory;
	config.RegisterFactory("ITimer", "ZPosixTimer", pstimerFactory);
	if (timerFactory == nullptr) {
		timerFactory = &pstimerFactory;
	}

	if (timerFactoryForFrame == nullptr) {
		timerFactoryForFrame = &pstimerFactory;
	}
#endif

	assert(timerFactory != nullptr);

#if !defined(CMAKE_PAINTSNOW) || ADD_THREAD_PTHREAD
	static const TFactory<ZThreadPthread, IThread> sthreadFactory;
	threadFactory = &sthreadFactory;
	config.RegisterFactory("IThread", "ZThreadPthread", sthreadFactory);
#endif

	assert(threadFactory != nullptr);

#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_OPENAL
	static const TFactory<ZAudioOpenAL, IAudio> saudioFactory;
	audioFactory = &saudioFactory;
	config.RegisterFactory("IAudio", "ZAudioOpenAL", saudioFactory);
#endif

	assert(audioFactory != nullptr);

#if !defined(CMAKE_PAINTSNOW) || ADD_ARCHIVE_DIRENT_BUILTIN
	static const TFactoryConstruct<ZArchiveDirent, IArchive> sarchiveFactory;

	archiveFactory = &sarchiveFactory;
	config.RegisterFactory("IArchive", "ZArchiveDirent", sarchiveFactory);
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_POD_BUILTIN
	static const TFactory<ZFilterPod, IFilterBase> sassetFilterFactory;
	assetFilterFactory = &sassetFilterFactory;
	config.RegisterFactory("IFilterBase::Asset", "ZFilterPod", sassetFilterFactory);
#else
	static const TFactory<NoFilter, IFilterBase> sfilterFactory;
	filterFactory = &sfilterFactory;
#endif

	assert(archiveFactory != nullptr);

	SetFactory(reinterpret_cast<const void*&>(renderFactory), paramRender, "IRender", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(frameFactory), paramFrame, "IFrame", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(threadFactory), paramThread, "IThread", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(audioFactory), paramAudio, "IAudio", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(archiveFactory), paramArchive, "IArchive", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(scriptFactory), paramScript, "IScript", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(networkFactory), paramNetwork, "INetwork", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(randomFactory), paramRandom, "IRandom", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(timerFactory), paramTimer, "ITimer", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(timerFactoryForFrame), paramTimerFrame, "ITimerForFrame", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(imageFactory), paramImage, "IImage", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(assetFilterFactory), paramAssetFilter, "IFilterBase::Asset", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(fontFactory), paramFont, "IFontBase", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(audioFilterFactory), paramAudio, "IFilterBase::Audio", factoryMap);
	SetFactory(reinterpret_cast<const void*&>(databaseFactory), paramDatabase, "IDatabase", factoryMap);

	thread = (*threadFactory)(paramThread);
	mainThread = thread->OpenCurrentThread();
	{
		frame = (*frameFactory)(paramFrame);

		IScript* script = (*scriptFactory)(paramScript);
		IRender* render = (*renderFactory)(paramRender);
		ITimer* timer = (*timerFactory)(paramTimer);
		IImage* image = (*imageFactory)(paramImage);
		INetwork* network = static_cast<INetwork*>((*networkFactory)(paramNetwork));
		ITunnel* tunnel = (*tunnelFactory)(paramTunnel);
		IAudio* audio = (*audioFactory)(paramAudio);
		IArchive* archive = (*archiveFactory)(paramArchive);
		IRandom* random = (*randomFactory)(paramRandom);
		IDatabase* database = (*databaseFactory)(paramDatabase);
		IFilterBase* assetFilter = (*assetFilterFactory)(paramAssetFilter);
		IFilterBase* audioFilter = (*audioFilterFactory)(paramAudioFilter);
		IFontBase* font = (*fontFactory)(paramFont);

		{
			Interfaces interfaces(*archive, *audio, *database, *assetFilter, *audioFilter, *font, *frame, *image, *network, *random, *render, *script, *thread, *timer, *tunnel);
			LeavesFlute leavesFlute(nogui, interfaces, subArchiveCreator, threadCount, warpCount);
			config.PostRuntimeState(&leavesFlute, LeavesApi::RUNTIME_INITIALIZE);
			this->leavesFlute = &leavesFlute;
			std::vector<String> paramList = Split(paramScript);
			leavesFlute.Execute(entry, paramList);

			std::map<String, CmdLine::Option>::const_iterator quit = configMap.find("Quit");
			if (quit == configMap.end() || (*quit).second.name != "true") {
				if (nogui) {
					leavesFlute.EnterStdinLoop();
				} else {
					leavesFlute.EnterMainLoop();
				}
			}

			// leavesFlute.Reset(false);
			config.PostRuntimeState(&leavesFlute, LeavesApi::RUNTIME_UNINITIALIZE);
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
#endif

Loader::Loader() : frameFactory(nullptr), renderFactory(nullptr), threadFactory(nullptr), audioFactory(nullptr), archiveFactory(nullptr), scriptFactory(nullptr), networkFactory(nullptr), timerFactory(nullptr), imageFactory(nullptr), assetFilterFactory(nullptr), fontFactory(nullptr), audioFilterFactory(nullptr), databaseFactory(nullptr), leavesFlute(nullptr) {
#ifdef _WIN32
	::CoInitialize(nullptr);
#endif
}

Loader::~Loader() {
#ifdef _WIN32
	::CoUninitialize();
#endif
}


