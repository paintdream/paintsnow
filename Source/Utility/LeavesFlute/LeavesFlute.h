// LeavesFlute.h
// The Leaves Wind
// By PaintDream (paintdream@paintdream.com)
// 2015-6-20
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "../../General/Interface/Interfaces.h"
#include "../../Core/Interface/IType.h"
#include "../../Utility/BridgeSunset/BridgeSunset.h"
#include "../../Utility/HeartVioliner/HeartVioliner.h"
#include "../../Utility/SnowyStream/SnowyStream.h"
#include "../../Utility/MythForest/MythForest.h"
#include "../../Utility/EchoLegend/EchoLegend.h"
#include "../../Utility/Remembery/Remembery.h"
#include "../../Utility/GalaxyWeaver/GalaxyWeaver.h"

namespace PaintsNow {
	class LeavesFlute : public TReflected<LeavesFlute, IScript::Library>, public ISyncObject, public IFrame::Callback {
	public:
		LeavesFlute(bool nogui, Interfaces& interfaces, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const String& defMount, uint32_t threadCount, uint32_t warpCount);
		~LeavesFlute() override;
		bool IsRendering() const override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		Interfaces& GetInterfaces() const;
		Kernel& GetKernel();

		// Script interfaces
		void Require(IScript::Request& request) override;
		void RequestListenConsole(IScript::Request& request, IScript::Request::Ref callback);
		void RequestPrint(IScript::Request& request, const String& text);
		void RequestExit(IScript::Request& request);
		void RequestWarpCursor(IScript::Request& request, Int2 position);
		void RequestShowCursor(IScript::Request& request, const String& type);
		void RequestSetAppTitle(IScript::Request& request, const String& title);
		void RequestSetScreenSize(IScript::Request& request, Int2& size);
		Int2 RequestGetScreenSize(IScript::Request& request);
		void RequestForward(IScript::Request& request, IScript::Request::Ref ref);
		void RequestInspect(IScript::Request& request, IScript::BaseDelegate d);
		void RequestSearchMemory(IScript::Request& request, const String& data, size_t start, size_t end, uint32_t alignment, uint32_t maxResult);

		void EnterMainLoop();
		void EnterStdinLoop();
		void BeginConsole();
		void EndConsole();
		bool ConsoleProc(IThread::Thread* thread, size_t index);
		bool ProcessCommand(const String& command);

		virtual void Execute(const String& file, const std::vector<String>& params);

	public:
		void OnInitialize(void* param) override;
		void OnRender() override;
		void OnWindowSize(const IFrame::EventSize&) override;
		void OnMouse(const IFrame::EventMouse& mouse) override;
		void OnKeyboard(const IFrame::EventKeyboard& keyboard) override;
		virtual void OnConsoleOutput(const String& text);
		virtual void Print(const String& text);

	protected:
		Interfaces& interfaces;
		std::vector<IScript::Library*> modules;

	public:
		BridgeSunset bridgeSunset;
		EchoLegend echoLegend;
		SnowyStream snowyStream;
		MythForest mythForest;
		HeartVioliner heartVioliner;
		Remembery remembery;
		GalaxyWeaver galaxyWeaver;

	protected:
		IThread::Thread* consoleThread;
		IScript::Request::Ref listenConsole;
		String newAppTitle;
		String appTitle;
	};
}

