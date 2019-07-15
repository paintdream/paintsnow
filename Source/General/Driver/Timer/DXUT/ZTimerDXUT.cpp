#ifdef CMAKE_PAINTSNOW

#include "ZTimerDXUT.h"


using namespace PaintsNow;

static UINT timerID = 0;

ZTimerDXUT::ZTimerDXUT() : interval(25), started(false), timerID(0) {
	
}

ZTimerDXUT::~ZTimerDXUT() {
}

void ZTimerDXUT::StopTimer() {
	if (started) {
		started = false;

		DXUTKillTimer(timerID);
	}
}

size_t ZTimerDXUT::GetTimerInterval() const {
	return interval;
}

void CALLBACK TimerProc(UINT timerID, void* context) {
	ZTimerDXUT* timer = reinterpret_cast<ZTimerDXUT*>(context);
	assert(timer != nullptr);

	timer->OnTimer();
}

void ZTimerDXUT::StartTimer(size_t inter, const TWrapper<void, size_t>& w) {
	wrapper = w;
	interval = inter;
	started = true;

	DXUTSetTimer(TimerProc, (float)inter / 1000.0f, &timerID, this);
}

bool ZTimerDXUT::IsTimerStarted() const {
	return started;
}

void ZTimerDXUT::OnTimer() {
	wrapper(interval);
}

#endif // CMAKE_PAINTSNOW
