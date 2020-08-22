// Engine.h
// By PaintDream (paintdream@paintdream.com)
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

		virtual ~Engine();
		void Clear();
		void InstallModule(Module* module);
		Module* GetComponentModuleFromName(const String& name) const;
		std::unordered_map<String, Module*>& GetModuleMap();
		void TickFrame();
		uint32_t GetFrameIndex() const;
		Kernel& GetKernel();
		void QueueFrameRoutine(ITask* task);
		IRender::Queue* GetWarpResourceQueue();

		Interfaces& interfaces;
		BridgeSunset& bridgeSunset;
		SnowyStream& snowyStream;

		void NotifyEntityConstruct(Entity* entity);
		void NotifyEntityDestruct(Entity* entity);
		void NotifyEntityAttach(Entity* child, Entity* parent);
		void NotifyEntityDetach(Entity* child);

	protected:
		Engine(const Engine& engine);
		Engine& operator = (const Engine& engine);

		std::atomic<uint32_t> frameIndex;
		std::atomic<uint32_t> entityCount;
		IThread::Event* finalizeEvent;
		std::unordered_map<String, Module*> modules;
		std::vector<TQueue<ITask*> > frameTasks;
		std::vector<IRender::Queue*> warpResourceQueues;

#ifdef _DEBUG
		std::unordered_map<Entity*, Entity*> entityMap;
		std::atomic<uint32_t> entityCritical;
#endif
	};

#define CHECK_THREAD_IN_MODULE(warpTiny) \
	(MUST_CHECK_REFERENCE_ONCE); \
	if (engine.GetKernel().GetCurrentWarpIndex() != warpTiny->GetWarpIndex()) { \
		request.Error("Threading routine failed on " #warpTiny); \
		assert(false); \
	}
}

