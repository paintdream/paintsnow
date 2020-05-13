// LeavesFlute.h
// The Leaves Wind
// By PaintDream (paintdream@paintdream.com)
// 2015-6-20
//

#ifndef __LEAVESFLUTE_H__
#define __LEAVESFLUTE_H__

#include "../../Core/PaintsNow.h"
#include "../../General/Interface/Interfaces.h"
#include "../../Core/Template/TFactory.h"
#include "../../Core/Interface/IType.h"
#include "../../Utility/BridgeSunset/BridgeSunset.h"
#include "../../Utility/FlameWork/FlameWork.h"
#include "../../Utility/HeartVioliner/HeartVioliner.h"
#include "../../Utility/SnowyStream/SnowyStream.h"
#include "../../Utility/MythForest/MythForest.h"
#include "../../Utility/EchoLegend/EchoLegend.h"
#include "../../Utility/Remembery/Remembery.h"
#include "../../Utility/GalaxyWeaver/GalaxyWeaver.h"

namespace PaintsNow {
	namespace NsLeavesFlute {
		class LeavesFlute : public TReflected<LeavesFlute, IScript::Library>, public ISyncObject, public IFrame::Callback {
		public:
			LeavesFlute(bool nogui, Interfaces& interfaces, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, uint32_t threadCount, uint32_t warpCount);
			virtual ~LeavesFlute();
			virtual bool IsRendering() const;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			Interfaces& GetInterfaces() const;
			Kernel& GetKernel();

			// Script interfaces
			virtual void Require(IScript::Request& request);
			void RequestListenConsole(IScript::Request& request, IScript::Request::Ref callback);
			void RequestPrint(IScript::Request& request, const String& text);
			void RequestExit(IScript::Request& request);
			void RequestGetFullPath(IScript::Request& request, const String& path);
			void RequestWarpCursor(IScript::Request& request, Int2 position);
			void RequestShowCursor(IScript::Request& request, const String& type);
			void RequestSetAppTitle(IScript::Request& request, const String& title);
			void RequestSetScreenSize(IScript::Request& request, Int2& size);
			void RequestGetScreenSize(IScript::Request& request);
			void RequestForward(IScript::Request& request, IScript::Request::Ref ref);
			void RequestInspect(IScript::Request& request, IScript::BaseDelegate d);

			void EnterMainLoop();
			void EnterStdinLoop();
			void BeginConsole();
			void EndConsole();
			bool ConsoleProc(IThread::Thread* thread, size_t index);
			bool ProcessCommand(const String& command);

			virtual void OnWindowSize(const PaintsNow::Int2&);
			virtual void Execute(const String& file, const std::vector<String>& params);

		public:
			virtual void OnInitialize(void* param);
			virtual void OnRender();
			virtual void OnMouse(const IFrame::EventMouse& mouse);
			virtual void OnKeyboard(const IFrame::EventKeyboard& keyboard);
			virtual void OnConsoleOutput(const String& text);
			virtual void Print(const String& text);

		protected:
			void ClearDynamicLibraryRefs();
			Interfaces& interfaces;
			std::vector<IScript::Library*> modules;

		public:
			NsBridgeSunset::BridgeSunset bridgeSunset;
			NsFlameWork::FlameWork flameWork;
			NsEchoLegend::EchoLegend echoLegend;
			NsSnowyStream::SnowyStream snowyStream;
			NsMythForest::MythForest mythForest;
			NsHeartVioliner::HeartVioliner heartVioliner;
			NsRemembery::Remembery remembery;
			NsGalaxyWeaver::GalaxyWeaver galaxyWeaver;

		protected:
			IThread::Thread* consoleThread;
			IScript::Request::Ref listenConsole;
			String newAppTitle;
			String appTitle;
		};
	}
}

#endif // __LEAVESFLUTE_H__
