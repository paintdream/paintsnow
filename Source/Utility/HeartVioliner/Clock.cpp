#include "Clock.h"
#include <ctime>
#include <iterator>
using namespace PaintsNow;

Clock::Clock(ITimer& base, BridgeSunset& b, int64_t interval, int64_t start, bool mergeTicks) : bridgeSunset(b), timerBase(base), now(start), offset(0) {
	if (mergeTicks) {
		Flag().fetch_or(CLOCK_MERGE_TICKS, std::memory_order_acquire);
	}

	Play();
	timer = base.StartTimer((size_t)interval, Wrap(this, &Clock::OnTimer));
}

void Clock::AddTicker(ITask* task, void* context) {
	tickerTasks[task] = context;
}

void Clock::RemoveTicker(ITask* task) {
	tickerTasks.erase(task);
}

Clock::~Clock() {
	assert(timer == nullptr);
	assert(tickerTasks.empty());
}

TObject<IReflect>& Clock::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(Now);
		ReflectMethod(Play);
		ReflectMethod(Pause);
		ReflectMethod(SetClock);
		ReflectMethod(Stop);
		ReflectMethod(IsRunning);
	}

	return *this;
}

void Clock::ScriptUninitialize(IScript::Request& request) {
	Stop();
	SharedTiny::ScriptUninitialize(request);
}

inline int64_t Clock::GetFullClock() const {
	return ITimer::GetSystemClock();
}

void Clock::Play() {
	Resume();
}

void Clock::Resume() {
	offset = GetFullClock() - now;
	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_acquire);
}

void Clock::Pause() {
	Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_release);
}

bool Clock::IsRunning() const {
	return !!(Flag() & TINY_ACTIVATED);
}

void Clock::SetClock(int64_t w) {
	now = w;
	offset = GetFullClock() - w;
}

int64_t Clock::Now() const {
	return now;
}

void Clock::Stop() {
	Pause();

	ITimer::Timer* t = timer;
	timer = nullptr;
	timerBase.StopTimer(t);
}

void Clock::Execute(void* context) {
	int64_t last = now;
	now = GetFullClock() - offset;
	std::vector<std::pair<ITask*, void*> > readyList;
	for (std::map<ITask*, void*>::iterator it = tickerTasks.begin(); it != tickerTasks.end(); ++it) {
		readyList.push_back(std::make_pair(it->first, it->second));
	}

	for (size_t i = 0; i < readyList.size(); i++) {
		readyList[i].first->Execute(readyList[i].second != nullptr ? readyList[i].second : context);
	}

	Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
}

void Clock::Abort(void* context) {}

void Clock::OnTimer(size_t interval) {
	if (timer != nullptr) {
		if (IsRunning()) {
			const FLAG flagMergeTick = CLOCK_MERGE_TICKS | TINY_MODIFIED;

			if ((Flag() & flagMergeTick) != flagMergeTick) {
				Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);
				bridgeSunset.GetKernel().QueueRoutine(this, this);
			}
		} else {
			offset = GetFullClock() - now;
		}
	}
}
