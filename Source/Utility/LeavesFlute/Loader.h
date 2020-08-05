// Loader.h
// By PaintDream (paintdream@paintdream.com)
// 2016-1-1
//

#ifndef __LOADER_H__
#define __LOADER_H__

#include "Config.h"
#include "CmdLine.h"

#if !defined(CMAKE_PAINTSNOW) || ADD_RENDER_OPENGL
#include "../../General/Driver/Render/OpenGL/ZRenderOpenGL.h"
#endif

#if (!defined(CMAKE_PAINTSNOW) || ADD_RENDER_VULKAN) && (!defined(_MSC_VER) || _MSC_VER > 1200)
#include "../../General/Driver/Render/Vulkan/ZRenderVulkan.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_LIBEVENT
#include "../../General/Driver/Network/LibEvent/ZNetworkLibEvent.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_IMAGE_FREEIMAGE
#include "../../General/Driver/Image/FreeImage/ZImageFreeImage.h"
#endif

#if defined(_WIN32) || defined(WIN32)
#include <Windows.h>
#if !defined(CMAKE_PAINTSNOW) || ADD_TIMER_TIMERQUEUE_BUILTIN
#include "../../General/Driver/Timer/WinTimerQueue/ZWinTimerQueue.h"
#endif
#else
#if !defined(CMAKE_PAINTSNOW) || ADD_TIMER_POSIX_BUILTIN
#include "../../General/Driver/Timer/PosixTimer/ZPosixTimer.h"
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FRAME_GLFW
#include "../../General/Driver/Frame/GLFW/ZFrameGLFW.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_ARCHIVE_DIRENT_BUILTIN
#include "../../Core/Driver/Archive/Dirent/ZArchiveDirent.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_LZMA_BUILTIN
#include "../../General/Driver/Archive/7Z/ZArchive7Z.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_OPENAL
#include "../../General/Driver/Audio/OpenAL/ZAudioOpenAL.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_LAME
#include "../../General/Driver/Filter/LAME/ZFilterLAME.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FONT_FREETYPE
#include "../../General/Driver/Font/Freetype/ZFontFreetype.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_SCRIPT_LUA_BUILTIN
#include "../../Core/Driver/Script/Lua/ZScriptLua.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_THREAD_PTHREAD
#include "../../Core/Driver/Thread/Pthread/ZThreadPthread.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_DATABASE_SQLITE3_BUILTIN
#include "../../General/Driver/Database/Sqlite/ZDatabaseSqlite.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_RANDOM_LIBNOISE_BUILTIN
#include "../../General/Driver/Random/Libnoise/ZRandomLibnoisePerlin.h"
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_POD_BUILTIN
#include "../../Core/Driver/Filter/Pod/ZFilterPod.h"
#endif

#include "../../Utility/LeavesFlute/LeavesFlute.h"


namespace PaintsNow {
	namespace NsLeavesFlute {
		class Loader {
		public:
			Loader();
			~Loader();
			void Load(const CmdLine& cmdLine);
			void SetFactory(const void*& ptr, String& param, const String& key, const std::map<String, CmdLine::Option>& factoryMap);

			const TFactoryBase<IFrame>* frameFactory;
			String paramFrame;
			const TFactoryBase<IRender>* renderFactory;
			String paramRender;
			const TFactoryBase<IThread>* threadFactory;
			String paramThread;
			const TFactoryBase<IAudio>* audioFactory;
			String paramAudio;
			const TFactoryBase<IArchive>* archiveFactory;
			String paramArchive;
			const TFactoryBase<IScript>* scriptFactory;
			String paramScript;
			const TFactoryBase<ITunnel>* networkFactory;
			String paramNetwork;
			const TFactoryBase<ITunnel>* tunnelFactory;
			String paramTunnel;
			const TFactoryBase<IRandom>* randomFactory;
			String paramRandom;
			const TFactoryBase<ITimer>* timerFactory;
			String paramTimer;
			const TFactoryBase<ITimer>* timerFactoryForFrame;
			String paramTimerFrame;
			const TFactoryBase<IImage>* imageFactory;
			String paramImage;
			const TFactoryBase<IFilterBase>* assetFilterFactory;
			String paramAssetFilter;
			const TFactoryBase<IFontBase>* fontFactory;
			String paramFont;
			const TFactoryBase<IFilterBase>* audioFilterFactory;
			String paramAudioFilter;
			const TFactoryBase<IDatabase>* databaseFactory;
			String paramDatabase;
			const TFactoryBase<IDebugger>* debuggerFactory;
			String paramDebugger;

			template <class T, class B>
			class FactoryInitWithThread : public TFactoryBase<B> {
			public:
				FactoryInitWithThread(Loader* l) : TFactoryBase<B>(Wrap(this, &FactoryInitWithThread<T, B>::CreateEx)), loader(l) {}

				B* CreateEx(const String& info = "") const {
					return new T(*loader->thread);
				}

			private:
				Loader* loader;
			};

			IThread::Thread* mainThread;
			IThread* thread;
			IFrame* frame;
			NsLeavesFlute::LeavesFlute* leavesFlute;
			Config config;
		};
	}
}


#endif // __LOADER_H__
