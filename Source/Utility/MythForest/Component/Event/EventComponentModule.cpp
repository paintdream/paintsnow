#include "EventComponentModule.h"
#include "../../Engine.h"
#include "../../../HeartVioliner/Clock.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include <iterator>

using namespace PaintsNow;

EventComponentModule::EventComponentModule(Engine& engine) : BaseClass(engine) {
	critical.store(0, std::memory_order_relaxed);
}

TObject<IReflect>& EventComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
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

	if (reflect.IsReflectEnum()) {
		Event::ReflectEventIDs(reflect);
	}

	return *this;
}

TShared<EventComponent> EventComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<EventComponent> eventComponent = TShared<EventComponent>::From(allocator->New());
	eventComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	engine.GetKernel().YieldCurrentWarp();

	return eventComponent;
}

void EventComponentModule::RequestBindEventTick(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, IScript::Delegate<Clock> clock) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	if (clock) {
		eventComponent->InstallTick(engine, clock.Get());
	} else {
		eventComponent->UninstallTick(engine);
	}
}

void EventComponentModule::RequestFilterEvent(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, uint32_t mask) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	uint32_t flag, old;
	do {
		flag = old = eventComponent->Flag().load(std::memory_order_acquire);
		flag = (flag & ~(EventComponent::EVENTCOMPONENT_ALL)) | (mask * EventComponent::EVENTCOMPONENT_BASE & EventComponent::EVENTCOMPONENT_ALL);
	} while (!eventComponent->Flag().compare_exchange_weak(old, flag, std::memory_order_release));
}

void EventComponentModule::RequestBindEventFrame(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, bool add) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	if (add) {
		eventComponent->InstallFrame();

		SpinLock(critical);
		std::vector<TShared<EventComponent> >::iterator it = std::find(frameTickers.begin(), frameTickers.end(), eventComponent.Get());
		if (it == frameTickers.end()) frameTickers.emplace_back(eventComponent.Get());
		SpinUnLock(critical);

	} else {
		SpinLock(critical);
		std::vector<TShared<EventComponent> >::iterator it = std::find(frameTickers.begin(), frameTickers.end(), eventComponent.Get());
		if (it != frameTickers.end()) frameTickers.erase(it);
		SpinUnLock(critical);

		eventComponent->UninstallFrame();
	}
}

void EventComponentModule::TickFrame() {
	SpinLock(critical);
	std::vector<TShared<EventComponent> > nextTickedListeners = frameTickers;
	SpinUnLock(critical);

	for (size_t j = 0; j < nextTickedListeners.size(); j++) {
		nextTickedListeners[j]->RoutineTickFrame(engine);
	}
}

void EventComponentModule::OnSize(const Int2& size) {
	SpinLock(critical);
	std::vector<TShared<EventComponent> > nextUserInputs = userInputs;
	SpinUnLock(critical);

	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnSize(engine, size);
	}
}

void EventComponentModule::OnMouse(const IFrame::EventMouse& mouse) {
	SpinLock(critical);
	std::vector<TShared<EventComponent> > nextUserInputs = userInputs;
	SpinUnLock(critical);

	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnMouse(engine, mouse);
	}
}

void EventComponentModule::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	SpinLock(critical);
	std::vector<TShared<EventComponent> > nextUserInputs = userInputs;
	SpinUnLock(critical);

	for (size_t i = 0; i < nextUserInputs.size(); i++) {
		nextUserInputs[i]->RoutineOnKeyboard(engine, keyboard);
	}
}

void EventComponentModule::RequestBindEventUserInput(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, bool enable) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	if (enable) {
		eventComponent->Flag().fetch_or((EventComponent::EVENTCOMPONENT_BASE << Event::EVENT_INPUT), std::memory_order_acquire);
		SpinLock(critical);
		std::vector<TShared<EventComponent> >::iterator it = std::binary_find(userInputs.begin(), userInputs.end(), eventComponent.Get());
		if (it == userInputs.end()) std::binary_insert(userInputs, eventComponent.Get());
		SpinUnLock(critical);
	} else {
		SpinLock(critical);
		std::vector<TShared<EventComponent> >::iterator it = std::binary_find(userInputs.begin(), userInputs.end(), eventComponent.Get());
		if (it != userInputs.end()) userInputs.erase(it);
		SpinUnLock(critical);
		eventComponent->Flag().fetch_and(~(EventComponent::EVENTCOMPONENT_BASE << Event::EVENT_INPUT), std::memory_order_release);
	}
}

void EventComponentModule::RequestBindEventNetwork(IScript::Request& request, IScript::Delegate<EventComponent> eventComponent, IScript::Delegate<Connection> connection) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(eventComponent);

	// TODO:
}

void EventComponentModule::Uninitialize() {
	SpinLock(critical);
	for (size_t i = 0; i < frameTickers.size(); i++) {
		frameTickers[i]->UninstallFrame();
	}

	frameTickers.clear();
	userInputs.clear();
	SpinUnLock(critical);
}