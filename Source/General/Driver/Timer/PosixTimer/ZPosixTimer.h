// ZPosixTimer.h]
// By PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma oncex

#include "../../../Interface/ITimer.h"

#include <cstdlib>
#include <signal.h>
#include <unistd.h>

namespace PaintsNow {
	class ZPosixTimer final : public ITimer {
	public:
		ZPosixTimer();
		virtual ~ZPosixTimer();

		virtual Timer* StartTimer(size_t interval, const TWrapper<void, size_t>& wrapper);
		virtual void StopTimer(Timer* timer);
		virtual size_t GetTimerInterval(Timer* ) const;

	};
}
