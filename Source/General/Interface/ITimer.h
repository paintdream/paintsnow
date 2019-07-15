// ITimer.h
// By PaintDream (paintdream@paintdream.com)
// 2014-12-14
//


#ifndef __ITIMER_H__
#define __ITIMER_H__


#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Template/TProxy.h"
#include "../../Core/Interface/IDevice.h"

namespace PaintsNow {
	class ITimer : public IDevice {
	public:
		virtual ~ITimer();
		struct Timer {};
		virtual Timer* StartTimer(size_t interval, const TWrapper<void, size_t>& wrapper) = 0;
		virtual void StopTimer(Timer* timer) = 0;
		virtual size_t GetTimerInterval(Timer* timer) const = 0;

		static int64_t GetSystemClock();
	};
}

#endif // __ITIMER_H__
