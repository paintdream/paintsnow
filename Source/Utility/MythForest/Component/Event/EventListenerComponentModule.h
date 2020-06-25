// EventListenerComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-16
//

#ifndef __EVENTLISTENERCOMPONENTMODULE_H__
#define __EVENTLISTENERCOMPONENTMODULE_H__

#include "EventListenerComponent.h"
#include "../../Module.h"
#include "../../../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {

	namespace NsHeartVioliner {
		class Clock;
	}
	namespace NsEchoLegend {
		class Connection;
	}
	namespace NsMythForest {
		class EventListenerComponent;
		class EventListenerComponentModule  : public TReflected<EventListenerComponentModule, ModuleImpl<EventListenerComponent> > {
		public:
			EventListenerComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void TickFrame() override;
			void OnSize(const Int2& size);
			void OnMouse(const IFrame::EventMouse& mouse);
			void OnKeyboard(const IFrame::EventKeyboard& keyboard);
			virtual void Uninitialize() override;

			TShared<EventListenerComponent> RequestNew(IScript::Request& request);
			void RequestSetEventHandler(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, IScript::Request::Ref callback);
			void RequestBindEventTick(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, IScript::Delegate<NsHeartVioliner::Clock> clock);
			void RequestBindEventFrame(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, bool enable);
			void RequestBindEventNetwork(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, IScript::Delegate<NsEchoLegend::Connection> connection);
			void RequestBindEventUserInput(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, bool enable);
			void RequestFilterEvent(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListnerComponent, uint32_t idMask);

		protected:
			std::vector<TShared<EventListenerComponent> > frameTickers;
			std::vector<TShared<EventListenerComponent> > userInputs;
			std::atomic<uint32_t> critical;
		};
	}
}

#endif // __EVENTLISTENERCOMPONENTMODULE_H__