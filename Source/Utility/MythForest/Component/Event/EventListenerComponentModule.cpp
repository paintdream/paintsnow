#include "EventListenerComponentModule.h"
#include "../../Engine.h"
#include "../../../HeartVioliner/Clock.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include <iterator>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::NsBridgeSunset;

EventListenerComponentModule::EventListenerComponentModule(Engine& engine) : BaseClass(engine) {
	critical.store(0, std::memory_order_relaxed);
}

TObject<IReflect>& EventListenerComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetEventHandler)[ScriptMethod = "SetEventHandler"];
		ReflectMethod(RequestBindEventTick)[ScriptMethod = "BindEventTick"];
		ReflectMethod(RequestBindEventFrame)[ScriptMethod = "BindEventFrame"];
		ReflectMethod(RequestBindEventUserInput)[ScriptMethod = "BindEventUserInput"];
		ReflectMethod(RequestBindEventNetwork)[ScriptMethod = "BindEventNetwork"];
		ReflectMethod(RequestFilterEvent)[ScriptMethod = "FilterEvent"];
	}

	if (reflect.IsReflectProperty()) {
		ReflectProperty(frameTickers)[Runtime];
		ReflectProperty(userInputs)[Runtime];
	}

	return *this;
}

void EventListenerComponentModule::RequestSetEventHandler(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, IScript::Request::Ref handler) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventListenerComponent);

	eventListenerComponent->SetCallback(request, handler);
}

TShared<EventListenerComponent> EventListenerComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<EventListenerComponent> eventListenerComponent = TShared<EventListenerComponent>::From(allocator->New());
	eventListenerComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	engine.GetKernel().YieldCurrentWarp();

	return eventListenerComponent;
}

void EventListenerComponentModule::RequestBindEventTick(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, IScript::Delegate<NsHeartVioliner::Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventListenerComponent);

	if (clock) {
		eventListenerComponent->InstallTick(engine, clock.Get());
	} else {
		eventListenerComponent->UninstallTick(engine);
	}
}

void EventListenerComponentModule::RequestFilterEvent(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, uint32_t mask) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventListenerComponent);

	uint32_t flag, old;
	do {
		flag = old = eventListenerComponent->Flag().load(std::memory_order_acquire);
		flag = (flag & ~(EventListenerComponent::EVENTLISTENER_ALL)) | (mask * EventListenerComponent::EVENTLISTENER_BASE & EventListenerComponent::EVENTLISTENER_ALL);
	} while (eventListenerComponent->Flag().compare_exchange_weak(old, flag, std::memory_order_release));
}

void EventListenerComponentModule::RequestBindEventFrame(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, bool add) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventListenerComponent);

	if (add) {
		eventListenerComponent->InstallFrame();

		SpinLock(critical);
		std::vector<TShared<EventListenerComponent> >::iterator it = std::find(frameTickers.begin(), frameTickers.end(), eventListenerComponent.Get());
		if (it == frameTickers.end()) frameTickers.emplace_back(eventListenerComponent.Get());
		SpinUnLock(critical);

	} else {
		SpinLock(critical);
		std::vector<TShared<EventListenerComponent> >::iterator it = std::find(frameTickers.begin(), frameTickers.end(), eventListenerComponent.Get());
		if (it != frameTickers.end()) frameTickers.erase(it);
		SpinUnLock(critical);

		eventListenerComponent->UninstallFrame();
	}
}

void EventListenerComponentModule::TickFrame() {
	SpinLock(critical);
	std::vector<TShared<EventListenerComponent> > nextTickedListeners = frameTickers;
	SpinUnLock(critical);

	for (size_t j = 0; j < nextTickedListeners.size(); j++) {
		nextTickedListeners[j]->RoutineTickFrame(engine);
	}
}

void EventListenerComponentModule::OnSize(const Int2& size) {
	SpinLock(critical);
	std::vector<TShared<EventListenerComponent> > nextUserInputs = userInputs;
	SpinUnLock(critical);

	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnSize(engine, size);
	}
}

void EventListenerComponentModule::OnMouse(const IFrame::EventMouse& mouse) {
	SpinLock(critical);
	std::vector<TShared<EventListenerComponent> > nextUserInputs = userInputs;
	SpinUnLock(critical);

	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnMouse(engine, mouse);
	}
}

void EventListenerComponentModule::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	SpinLock(critical);
	std::vector<TShared<EventListenerComponent> > nextUserInputs = userInputs;
	SpinUnLock(critical);

	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnKeyboard(engine, keyboard);
	}
}

void EventListenerComponentModule::RequestBindEventUserInput(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, bool enable) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventListenerComponent);

	if (enable) {
		eventListenerComponent->Flag() |= (EventListenerComponent::EVENTLISTENER_BASE << Event::EVENT_INPUT);
		SpinLock(critical);
		std::vector<TShared<EventListenerComponent> >::iterator it = std::find(userInputs.begin(), userInputs.end(), eventListenerComponent.Get());
		if (it == userInputs.end()) userInputs.emplace_back(eventListenerComponent.Get());
		SpinUnLock(critical);
	} else {
		SpinLock(critical);
		std::vector<TShared<EventListenerComponent> >::iterator it = std::find(userInputs.begin(), userInputs.end(), eventListenerComponent.Get());
		if (it != userInputs.end()) userInputs.erase(it);
		SpinUnLock(critical);
		eventListenerComponent->Flag() &= ~(EventListenerComponent::EVENTLISTENER_BASE << Event::EVENT_INPUT);
	}
}

void EventListenerComponentModule::RequestBindEventNetwork(IScript::Request& request, IScript::Delegate<EventListenerComponent> eventListenerComponent, IScript::Delegate<NsEchoLegend::Connection> connection) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventListenerComponent);

	// TODO:
}

void EventListenerComponentModule::Uninitialize() {
	SpinLock(critical);
	for (size_t i = 0; i < frameTickers.size(); i++) {
		frameTickers[i]->UninstallFrame();
	}

	frameTickers.clear();
	userInputs.clear();
	SpinUnLock(critical);
}