// EventComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-16
//

#ifndef __EVENTCOMPONENT_H__
#define __EVENTCOMPONENT_H__

#include "../../../../Core/Template/TEvent.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../Component.h"

namespace PaintsNow {
	namespace NsHeartVioliner {
		class Clock;
	}
	namespace NsMythForest {
		class EventComponent : public TAllocatedTiny<EventComponent, Component>, public TaskRepeat {
		public:
			EventComponent();

			void RoutineOnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard);
			void RoutineOnMouse(Engine& engine, const IFrame::EventMouse& mouse);
			void RoutineOnSize(Engine& engine, const Int2& size);
			void RoutineTickFrame(Engine& engine);

			void OnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard);
			void OnMouse(Engine& engine, const IFrame::EventMouse& mouse);
			void OnSize(Engine& engine, const IFrame::EventSize& size);
			void InstallTick(Engine& engine, TShared<NsHeartVioliner::Clock> clock);
			void UninstallTick(Engine& engine);
			void InstallFrame();
			void UninstallFrame();

			enum {
				EVENTCOMPONENT_UPDATING_FRAMELIST = COMPONENT_CUSTOM_BEGIN,
				EVENTCOMPONENT_INSTALLED_FRAME = COMPONENT_CUSTOM_BEGIN << 1,
				EVENTCOMPONENT_INSTALLED_TICK = COMPONENT_CUSTOM_BEGIN << 2,
				EVENTCOMPONENT_BASE = COMPONENT_CUSTOM_BEGIN << 3,
				EVENTCOMPONENT_END = EVENTCOMPONENT_BASE << (Event::EVENT_END - Event::EVENT_SPECIAL_BEGIN),
				EVENTCOMPONENT_ALL = EVENTCOMPONENT_END - EVENTCOMPONENT_BASE,
				EVENTCOMPONENT_CUSTOM_BEGIN = EVENTCOMPONENT_BASE << Event::EVENT_END
			};

			virtual Entity* GetHostEntity() const override;
			virtual FLAG GetEntityFlagMask() const override;
			virtual void Initialize(Engine& engine, Entity* entity) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
			virtual void Execute(void* context) override;
			virtual void Abort(void* context) override;

			uint64_t GetTickDeltaTime() const;
			virtual TObject<IReflect>& operator () (IReflect& reflect);

		protected:
			// Since we can't iterate entity's components in render thread,
			// Apply an asynchronized way to collect components instead.
			void RoutineSetupFrameTickers(Engine& engines);

		protected:
			std::vector<TShared<Component> > frameTickerCollection;
			TShared<NsHeartVioliner::Clock> clock;
			std::atomic<int32_t> critical;
			Entity* rootEntity;
			uint64_t tickTimeStamp;
			uint64_t tickTimeDelta;
		};
	}
}

#endif // __EVENTCOMPONENT_H__