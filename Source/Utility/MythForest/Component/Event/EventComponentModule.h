// EventComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-16
//

#ifndef __EVENTCOMPONENTMODULE_H__
#define __EVENTCOMPONENTMODULE_H__

#include "EventComponent.h"
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
		class EventComponent;
		class EventComponentModule  : public TReflected<EventComponentModule, ModuleImpl<EventComponent> > {
		public:
			EventComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void TickFrame() override;
			void OnSize(const Int2& size);
			void OnMouse(const IFrame::EventMouse& mouse);
			void OnKeyboard(const IFrame::EventKeyboard& keyboard);
			virtual void Uninitialize() override;

			TShared<EventComponent> RequestNew(IScript::Request& request);
			void RequestBindEventTick(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, IScript::Delegate<NsHeartVioliner::Clock> clock);
			void RequestBindEventFrame(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, bool enable);
			void RequestBindEventNetwork(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, IScript::Delegate<NsEchoLegend::Connection> connection);
			void RequestBindEventUserInput(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, bool enable);
			void RequestFilterEvent(IScript::Request& request, IScript::Delegate<EventComponent> eventListnerComponent, uint32_t idMask);

		protected:
			std::vector<TShared<EventComponent> > frameTickers;
			std::vector<TShared<EventComponent> > userInputs;
			std::atomic<uint32_t> critical;
		};
	}
}

#endif // __EVENTCOMPONENTMODULE_H__