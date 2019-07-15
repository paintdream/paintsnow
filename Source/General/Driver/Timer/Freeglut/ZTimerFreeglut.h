// ZTimerFreeglut.h
// By PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#ifndef __ZTIMERFREEGLUT_H__
#define __ZTIMERFREEGLUT_H__

#include "../../../Interface/ITimer.h"

namespace PaintsNow {
	class ZTimerFreeglut final : public ITimer {
	public:
		ZTimerFreeglut();
		virtual ~ZTimerFreeglut();

		virtual Timer* StartTimer(size_t interval, const TWrapper<void, size_t>& wrapper) override;
		virtual void StopTimer(Timer* timer) override;
		virtual size_t GetTimerInterval(Timer* timer) const override;
	};
}

#endif // __ZTIMERFREEGLUT__
