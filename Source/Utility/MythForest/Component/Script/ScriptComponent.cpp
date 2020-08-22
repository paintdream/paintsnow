#include "ScriptComponent.h"
#include "../../Entity.h"
#include "../../Engine.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../Event/EventComponent.h"

using namespace PaintsNow;

ScriptComponent::ScriptComponent(const String& n) : name(n), entityFlagMask(0) {
	memset(handlers, 0, sizeof(handlers));
}

ScriptComponent::~ScriptComponent() {}

void ScriptComponent::Uninitialize(Engine& engine, Entity* entity) {
	IScript::Request& request = engine.interfaces.script.GetDefaultRequest();
	request.DoLock();
	for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
		if (handlers[i]) {
			request.Dereference(handlers[i]);
			handlers[i] = IScript::Request::Ref();
		}
	}
	request.UnLock();

	BaseClass::Uninitialize(engine, entity);
}

const String& ScriptComponent::GetAliasedTypeName() const {
	return name;
}

void ScriptComponent::SetHandler(IScript::Request& request, Event::EVENT_ID event, IScript::Request::Ref handler) {
	std::swap(handlers[event], handler);

	if (handler) {
		request.DoLock();
		request.Dereference(handler);
		request.UnLock();
	}

	UpdateEntityFlagMask();
}

namespace PaintsNow {
	IScript::Request& operator << (IScript::Request& request, const IFrame::EventSize& size) {
		return request << begintable << key("Size") << size.size << endtable;
	}

	IScript::Request& operator << (IScript::Request& request, const IFrame::EventKeyboard& keyboard) {
		request << begintable << key("Keyboard") << begintable
			<< key("Name") << String(keyboard.GetName())
			<< key("Down") << !(keyboard.keyCode & IFrame::EventKeyboard::KEY_POP)
			<< endtable << endtable;
		return request;
	}

	IScript::Request& operator << (IScript::Request& request, const IFrame::EventMouse& mouse) {
		request << begintable << key("Mouse") << begintable
			<< key("Button") << mouse.left
			<< key("Down") << mouse.down
			<< key("Move") << mouse.move
			<< key("Wheel") << mouse.wheel
			<< key("Position") << mouse.position
			<< endtable << endtable;
		return request;
	}
}

Tiny::FLAG ScriptComponent::GetEntityFlagMask() const {
	// return Entity::ENTITY_HAS_TICK_EVENT | Entity::ENTITY_HAS_PREPOST_TICK_EVENT;
	return entityFlagMask;
}

void ScriptComponent::UpdateEntityFlagMask() {
	entityFlagMask = (handlers[Event::EVENT_ATTACH_COMPONENT] || handlers[Event::EVENT_DETACH_COMPONENT]
		? Entity::ENTITY_HAS_TACH_EVENT : 0)
		| (handlers[Event::EVENT_ENTITY_ACTIVATE] || handlers[Event::EVENT_ENTITY_DEACTIVATE] ? Entity::ENTITY_HAS_ACTIVE_EVENT : 0)
		| (handlers[Event::EVENT_PRETICK] || handlers[Event::EVENT_POSTTICK] ? Entity::ENTITY_HAS_PREPOST_TICK_EVENT : 0)
		| (handlers[Event::EVENT_TICK] ? Entity::ENTITY_HAS_TICK_EVENT : 0)
		| (handlers[Event::EVENT_CUSTOM] || handlers[Event::EVENT_INPUT] || handlers[Event::EVENT_OUTPUT] || handlers[Event::EVENT_FRAME] || handlers[Event::EVENT_FRAME_SYNC_TICK] ? Entity::ENTITY_HAS_SPECIAL_EVENT : 0);
}

void ScriptComponent::DispatchEvent(Event& event, Entity* entity) {
	IScript::Request::Ref handler = handlers[event.eventID];
	if (handler) {
		Engine& engine = event.engine;
		IScript::RequestPool& requestPool = engine.bridgeSunset;
		IScript::Request& request = *requestPool.AcquireSafe();
		request.DoLock();
		request.Push();

		// Translate message
		switch (event.eventID) {
			case Event::EVENT_PRETICK:
			case Event::EVENT_TICK:
			case Event::EVENT_POSTTICK:
			{
				EventComponent* eventComponent = event.sender->QueryInterface(UniqueType<EventComponent>());
				request << eventComponent->GetTickDeltaTime();
				break;
			}
			case Event::EVENT_ATTACH_COMPONENT:
			case Event::EVENT_DETACH_COMPONENT:
			case Event::EVENT_ENTITY_ACTIVATE:
			case Event::EVENT_ENTITY_DEACTIVATE:
			{
				request << event.sender << event.detail;
				break;
			}
			case Event::EVENT_INPUT:
			{
				TShared<Event::Wrapper<IFrame::EventKeyboard> > keyboard = event.detail->QueryInterface(UniqueType<Event::Wrapper<IFrame::EventKeyboard> >());
				if (keyboard) {
					request << keyboard->data;
				} else {
					TShared<Event::Wrapper<IFrame::EventMouse> > mouse = event.detail->QueryInterface(UniqueType<Event::Wrapper<IFrame::EventMouse> >());
					if (mouse) {
						request << mouse->data;
					} else {
						TShared<Event::Wrapper<IFrame::EventSize> > size = event.detail->QueryInterface(UniqueType<Event::Wrapper<IFrame::EventSize> >());
						if (size) {
							request << size->data;
						}
					}
				}
				break;
			}
		}

		request.Call(sync, handler);
		request.Pop();
		request.UnLock();
		requestPool.ReleaseSafe(&request);
	}
}
