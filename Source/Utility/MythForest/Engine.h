// Engine.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-1
//

#pragma once
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../Core/System/Tiny.h"
#include "../../Core/Template/TMap.h"
#include "../../General/Interface/Interfaces.h"
#include "../../Core/System/Kernel.h"
#include <queue>

namespace PaintsNow {
	class Interfaces;
	class BridgeSunset;
	class SnowyStream;
	class Unit;
	class Entity;
	class Module;
	class Engine : public ISyncObject {
	public:
		Engine(Interfaces& interfaces, BridgeSunset& bridgeSunset, SnowyStream& snowyStream);

		~Engine() override;
		void Clear();
		void InstallModule(Module* module);
		Module* GetComponentModuleFromName(const String& name) const;
		std::unordered_map<String, Module*>& GetModuleMap();
		void TickFrame();
		uint32_t GetFrameIndex() const;
		Kernel& GetKernel();
		void QueueFrameRoutine(ITask* task, const TShared<SharedTiny>& tiny);
		IRender::Queue* GetWarpResourceQueue();

		Interfaces& interfaces;
		BridgeSunset& bridgeSunset;
		SnowyStream& snowyStream;

		void NotifyUnitConstruct(Unit* unit);
		void NotifyUnitDestruct(Unit* unit);
		void NotifyUnitAttach(Unit* child, Unit* parent);
		void NotifyUnitDetach(Unit* child);

	protected:
		Engine(const Engine& engine);
		Engine& operator = (const Engine& engine);

		std::atomic<uint32_t> frameIndex;
		std::atomic<uint32_t> unitCount;
		IThread::Event* finalizeEvent;
		std::unordered_map<String, Module*> modules;
		std::vector<std::queue<std::pair<ITask*, TShared<SharedTiny> > > > frameTasks;
		std::vector<IRender::Queue*> warpResourceQueues;

#ifdef _DEBUG
		std::map<Unit*, Unit*> entityMap;
		std::atomic<uint32_t> unitCritical;
#endif
	};

#define CHECK_THREAD_IN_MODULE(warpTiny) \
	(MUST_CHECK_REFERENCE_ONCE); \
	if (engine.GetKernel().GetCurrentWarpIndex() != warpTiny->GetWarpIndex()) { \
		request.Error("Threading routine failed on " #warpTiny); \
		assert(false); \
	}
}

