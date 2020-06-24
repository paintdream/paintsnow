#include "EventListenerComponent.h"
#include "../../Entity.h"
#include "../../../HeartVioliner/Clock.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsHeartVioliner;
using namespace PaintsNow::NsBridgeSunset;

class Reflector : public IReflect {
public:
	Reflector() : IReflect(false, false, false, true) {}

	virtual void Enum(size_t value, Unique id, const char* name, const MetaChainBase* meta) override {
		EventIDToNames.resize(value + 1);
		EventIDToNames[value] = name;
	}

	std::vector<String> EventIDToNames;
} TheReflector;

struct Initer {
	Initer() {
		Event::ReflectEventIDs(TheReflector);
	}
};

EventListenerComponent::EventListenerComponent() : rootEntity(nullptr), tickTimeStamp(0), tickTimeDelta(0) {
	Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_acquire);
	critical.store(0, std::memory_order_relaxed);

	static Initer initer;
}

Tiny::FLAG EventListenerComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT;
}

void EventListenerComponent::Abort(void* context) {

}

uint64_t EventListenerComponent::GetTickDeltaTime() const {
	return tickTimeDelta;
}

void EventListenerComponent::Execute(void* context) {
	if (rootEntity != nullptr) {
		uint64_t time = ITimer::GetSystemClock();
		tickTimeDelta = time - tickTimeStamp;

		Event event(*reinterpret_cast<Engine*>(context), Event::EVENT_TICK, rootEntity, this);
		bool prepost = !!(rootEntity->Flag() & Entity::ENTITY_HAS_PREPOST_TICK_EVENT);

		if (prepost) {
			event.eventID = Event::EVENT_PRETICK;
			rootEntity->PostEvent(event);
			event.eventID = Event::EVENT_TICK;
		}

		if (rootEntity->Flag() & Entity::ENTITY_HAS_TICK_EVENT) {
			rootEntity->PostEvent(event);
		}

		if (prepost) {
			event.eventID = Event::EVENT_POSTTICK;
			rootEntity->PostEvent(event);
		}

		tickTimeStamp = time;
	}
}

void EventListenerComponent::InstallFrame() {
	Flag().fetch_or(EVENTLISTENER_INSTALLED_FRAME, std::memory_order_acquire);

}

void EventListenerComponent::UninstallFrame() {
	Flag().fetch_and(~EVENTLISTENER_INSTALLED_FRAME, std::memory_order_release);
	SpinLock(critical);
	frameTickerCollection.clear();
	SpinUnLock(critical);}


void EventListenerComponent::InstallTick(Engine& engine, TShared<Clock> c) {
	assert(c->GetWarpIndex() == GetWarpIndex());
	if (clock) {
		UninstallTick(engine);
	}

	if (!(Flag() & EVENTLISTENER_INSTALLED_TICK)) {
		Flag().fetch_or((EVENTLISTENER_INSTALLED_TICK | (EVENTLISTENER_BASE << Event::EVENT_TICK)), std::memory_order_acquire);
		clock = c;
		tickTimeStamp = ITimer::GetSystemClock();

		ReferenceObject();
		clock->AddTicker(this, &engine);
	}
}

void EventListenerComponent::UninstallTick(Engine& engine) {
	if (Flag() & EVENTLISTENER_INSTALLED_TICK) {
		clock->RemoveTicker(this);
		clock = nullptr;
		Flag().fetch_and(~(EVENTLISTENER_INSTALLED_TICK | (EVENTLISTENER_BASE << Event::EVENT_TICK)), std::memory_order_release);
		ReleaseObject();
	}
}

void EventListenerComponent::ScriptUninitialize(IScript::Request& request) {
	if (callback) {
		request.DoLock();
		request.Dereference(callback);
		request.UnLock();
	}

	BaseClass::ScriptUninitialize(request);
}

void EventListenerComponent::SetCallback(IScript::Request& request, IScript::Request::Ref ref) {
	if (callback) {
		request.Dereference(callback);
	}

	callback = ref;
}

IScript::Request::Ref EventListenerComponent::GetCallback() const {
	return callback;
}

void EventListenerComponent::DispatchEvent(Event& event, Entity* entity) {
	if (Flag() & (EventListenerComponent::EVENTLISTENER_BASE << event.eventID)) {
		Engine& engine = event.engine;
		if (callback) {
			if (event.eventID == Event::EVENT_TICK) {
				engine.GetKernel().QueueRoutine(this, CreateTaskScript(callback, TheReflector.EventIDToNames[event.eventID], GetTickDeltaTime()));
			} else {
				engine.GetKernel().QueueRoutine(this, CreateTaskScript(callback, TheReflector.EventIDToNames[event.eventID], event.eventMeta));
			}
		}
	}
}

void EventListenerComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);
	// enables all event slots ...
	assert(rootEntity == nullptr);
	rootEntity = entity;
}

void EventListenerComponent::Uninitialize(Engine& engine, Entity* entity) {
	assert(rootEntity != nullptr);
	UninstallFrame();
	UninstallTick(engine);
	rootEntity = nullptr;
	BaseClass::Uninitialize(engine, entity);
}

void EventListenerComponent::RoutineSetupFrameTickers(Engine& engine) {
	if (!(Flag() & EVENTLISTENER_INSTALLED_FRAME)) return;

	std::vector<TShared<Component> > nextTickerCollection;
	if (rootEntity != nullptr) {
		Event eventSyncTickFrame(engine, Event::EVENT_FRAME_SYNC_TICK, this);
		const std::vector<Component*>& components = rootEntity->GetComponents();
		nextTickerCollection.reserve(components.size());
		for (size_t i = 0; i < components.size(); i++) {
			Component* component = components[i];
			if (component != nullptr && component != this) {
				component->DispatchEvent(eventSyncTickFrame, rootEntity);
				nextTickerCollection.emplace_back(component);
			}
		}
	}

	SpinLock(critical);
	if (Flag() & EVENTLISTENER_INSTALLED_FRAME) {
		std::swap(frameTickerCollection, nextTickerCollection);
	}
	SpinUnLock(critical);
}

void EventListenerComponent::RoutineOnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard) {
	engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &EventListenerComponent::OnKeyboard), std::ref(engine), keyboard));
}

void EventListenerComponent::RoutineOnMouse(Engine& engine, const IFrame::EventMouse& mouse) {
	engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &EventListenerComponent::OnMouse), std::ref(engine), mouse));
}

void EventListenerComponent::RoutineOnSize(Engine& engine, const Int2& size) {
	engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &EventListenerComponent::OnSize), std::ref(engine), size));
}


namespace PaintsNow {
	struct EventSize {
		EventSize() {}
		EventSize(const Int2& s) : size(s) {}

		Int2 size;
	};

	IScript::Request& operator << (IScript::Request& request, const EventSize& size) {
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

void EventListenerComponent::OnKeyboard(Engine& engine, const IFrame::EventKeyboard& keyboard) {
	if (callback) {
		engine.GetKernel().QueueRoutine(this, CreateTaskScript(callback, TheReflector.EventIDToNames[Event::EVENT_INPUT], keyboard));
	}
}

void EventListenerComponent::OnMouse(Engine& engine, const IFrame::EventMouse& mouse) {
	if (callback) {
		engine.GetKernel().QueueRoutine(this, CreateTaskScript(callback, TheReflector.EventIDToNames[Event::EVENT_INPUT], mouse));
	}
}

void EventListenerComponent::OnSize(Engine& engine, const Int2& size) {
	if (callback) {
		engine.GetKernel().QueueRoutine(this, CreateTaskScript(callback, TheReflector.EventIDToNames[Event::EVENT_INPUT], EventSize(size)));
	}
}

void EventListenerComponent::RoutineTickFrame(Engine& engine) {
	Event event(engine, Event::EVENT_FRAME, rootEntity);
	// Do not post them directly because we are in render thread
	// rootEntity->PostEvent(event);
	engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &EventListenerComponent::RoutineSetupFrameTickers), std::ref(engine)));

	// may be previous frame's collection
	SpinLock(critical);
	std::vector<TShared<Component> > components = frameTickerCollection;
	SpinUnLock(critical);

	for (size_t i = 0; i < components.size(); i++) {
		components[i]->DispatchEvent(event, nullptr);
	}
}

TObject<IReflect>& EventListenerComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(rootEntity)[Runtime];
		ReflectProperty(frameTickerCollection)[Runtime];
	}

	return *this;
}
