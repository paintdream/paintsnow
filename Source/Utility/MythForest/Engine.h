// Engine.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-1
//

#ifndef __ENGINE_H__
#define __ENGINE_H__

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <set>
#include "../../Core/System/Tiny.h"
#include "../../Core/Template/TMap.h"
#include "../../General/Interface/Interfaces.h"
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	class Interfaces;
	namespace NsBridgeSunset {
		class BridgeSunset;
	}
	namespace NsSnowyStream {
		class SnowyStream;
	}
	namespace NsMythForest {
		class Unit;
		class Entity;
		class MythForest;
		class Module;
		class Engine : public ISyncObject {
		public:
			Engine(Interfaces& interfaces, NsBridgeSunset::BridgeSunset& bridgeSunset, NsSnowyStream::SnowyStream& snowyStream, NsMythForest::MythForest& mythForest);

			virtual ~Engine();
			void Clear();
			void InstallModule(Module* module);
			void UninstallModule(Module* module);
			Module* GetComponentModuleFromName(const String& name) const;
			const unordered_map<String, Module*>& GetModuleMap() const;
			void TickFrame();
			uint32_t GetFrameIndex() const;
			Kernel& GetKernel();
			void QueueFrameRoutine(ITask* task);

			Interfaces& interfaces;
			NsBridgeSunset::BridgeSunset& bridgeSunset;
			NsSnowyStream::SnowyStream& snowyStream;
			NsMythForest::MythForest& mythForest;

		protected:
			friend class Entity;
			inline void NotifyEntityConstruct() {
				entityCount.fetch_add(1, std::memory_order_release);
			}

			inline void NotifyEntityDestruct() {
				if (entityCount.fetch_sub(1, std::memory_order_release) == 1) {
					interfaces.thread.Signal(finalizeEvent, false);
				}
			}

		protected:
			Engine(const Engine& engine);
			Engine& operator = (const Engine& engine);

			TAtomic<uint32_t> frameIndex;
			TAtomic<uint32_t> entityCount;
			IThread::Event* finalizeEvent;
			unordered_map<String, Module*> modules;
			std::vector<TQueue<ITask*> > frameTasks;
		};

#define CHECK_THREAD_IN_MODULE(warpTiny) \
	(MUST_CHECK_REFERENCE_ONCE); \
	if (engine.GetKernel().GetCurrentWarpIndex() != warpTiny->GetWarpIndex()) { \
		request.Error("Threading routine failed on " #warpTiny); \
		assert(false); \
	}
	}
}

#endif // __ENGINE_H__