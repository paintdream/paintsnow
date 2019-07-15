#include "ZTimerFreeglut.h"

#define FREEGLUT_STATIC
#include <GL/freeglut.h>
#include <cassert>
// #include <unordered_map>
#include <map>

using namespace PaintsNow;

/*
#if !(defined(_MSC_VER) && _MSC_VER <= 1200)
typedef unordered_map<int, ZTimerFreeglut*> hmap;
#else
struct HashFunc {
	size_t operator () (int ch) const {
		return (size_t)(((ch << 1) & 0x7fff) | (ch & 0x8000));
	}
};

typedef unordered_map<int, ZTimerFreeglut*, HashFunc> hmap;
#endif*/

struct TimerFreeglutImpl : public ITimer::Timer {
	TWrapper<void, size_t> wrapper;
	size_t interval;
	int timerID;
	bool started;
};


typedef std::map<int, TimerFreeglutImpl*> hmap;
hmap mapTimers;

static void _TimerFunc(int);
static void _TimerFunc(int value)
{
	hmap::iterator it = mapTimers.find(value);
	if (it == mapTimers.end())
		return;
	TimerFreeglutImpl* t = (*it).second;
	
	glutTimerFunc((int)t->interval, _TimerFunc, value);
	t->wrapper(t->interval);
}

ZTimerFreeglut::ZTimerFreeglut() {}
ZTimerFreeglut::~ZTimerFreeglut() {}

void ZTimerFreeglut::StopTimer(Timer* timer) {
	TimerFreeglutImpl* impl = static_cast<TimerFreeglutImpl*>(timer);
	mapTimers.erase(impl->timerID);
	delete impl;
}

size_t ZTimerFreeglut::GetTimerInterval(Timer* timer) const {
	TimerFreeglutImpl* impl = static_cast<TimerFreeglutImpl*>(timer);
	return impl->interval;
}

ITimer::Timer* ZTimerFreeglut::StartTimer(size_t inter, const TWrapper<void, size_t>& w) {
	static int globalID = 0;
	TimerFreeglutImpl* impl = new TimerFreeglutImpl();
	while (mapTimers.find(globalID) != mapTimers.end()) {
		globalID++;
	}

	impl->timerID = globalID;
	mapTimers[impl->timerID] = impl;

	impl->wrapper = w;
	impl->interval = inter;
	impl->started = true;

	glutTimerFunc((int)inter, _TimerFunc, impl->timerID);

	return impl;
}