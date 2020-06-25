// EventListenerComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-16
//

#ifndef __EVENTLISTENERCOMPONENT_H__
#define __EVENTLISTENERCOMPONENT_H__

#include "../../../../Core/Template/TEvent.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../Component.h"

namespace PaintsNow {
	namespace NsHeartVioliner {
		class Clock;
	}
	namespace NsMythForest {
		class EventListenerComponent : public TAllocatedTiny<EventListenerComponent, Component>, public TaskRepeat {
		public:
			EventListenerComponent();

			void RoutineOnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard);
			void RoutineOnMouse(Engine& engine, const IFrame::EventMouse& mouse);
			void RoutineOnSize(Engine& engine, const Int2& size);
			void RoutineTickFrame(Engine& engine);

			void OnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard);
			void OnMouse(Engine& engine, const IFrame::EventMouse& mouse);
			void OnSize(Engine& engine, const Int2& size);
			void InstallTick(Engine& engine, TShared<NsHeartVioliner::Clock> clock);
			void UninstallTick(Engine& engine);
			void InstallFrame();
			void UninstallFrame();

			enum {
				EVENTLISTENER_UPDATING_FRAMELIST = COMPONENT_CUSTOM_BEGIN,
				EVENTLISTENER_INSTALLED_FRAME = COMPONENT_CUSTOM_BEGIN << 1,
				EVENTLISTENER_INSTALLED_TICK = COMPONENT_CUSTOM_BEGIN << 2,
				EVENTLISTENER_BASE = COMPONENT_CUSTOM_BEGIN << 3,
				EVENTLISTENER_END = EVENTLISTENER_BASE << Event::EVENT_END,
				EVENTLISTENER_ALL = EVENTLISTENER_END - EVENTLISTENER_BASE,
				EVENTLISTENER_CUSTOM_BEGIN = EVENTLISTENER_BASE << Event::EVENT_END
			};

			virtual FLAG GetEntityFlagMask() const override;
			virtual void Initialize(Engine& engine, Entity* entity) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
			virtual void Execute(void* context) override;
			virtual void Abort(void* context) override;

			void SetCallback(IScript::Request& request, IScript::Request::Ref ref);
			uint64_t GetTickDeltaTime() const;
			IScript::Request::Ref GetCallback() const;

			virtual TObject<IReflect>& operator () (IReflect& reflect);

		protected:
			// Since we can't iterate entity's components in render thread,
			// Apply an asynchronized way to collect components instead.
			void RoutineSetupFrameTickers(Engine& engines);

		protected:
			virtual void DispatchEvent(Event& event, Entity* entity) override;
			virtual void ScriptUninitialize(IScript::Request& request) override;

			std::vector<TShared<Component> > frameTickerCollection;
			IScript::Request::Ref callback;
			TShared<NsHeartVioliner::Clock> clock;
			std::atomic<int32_t> critical;
			Entity* rootEntity;
			uint64_t tickTimeStamp;
			uint64_t tickTimeDelta;
		};
	}
}

#endif // __EVENTLISTENERCOMPONENT_H__