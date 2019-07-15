// ZTimerDXUT.h
// By PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#ifndef __ZTIMERDXUT_H__
#define __ZTIMERDXUT_H__

#ifdef CMAKE_PAINTSNOW

#include "../../../Interface/ITimer.h"
#include "../../Frame/DXUT/Core/DXUT.h"
#include "../../Frame/DXUT/Core/DXUTcamera.h"
#include "../../Frame/DXUT/Core/SDKmisc.h"

namespace PaintsNow {
	class ZTimerDXUT final : public ITimer {
	public:
		ZTimerDXUT();
		virtual ~ZTimerDXUT();

		virtual void StartTimer(size_t interval, const TWrapper<void, size_t>& wrapper);
		virtual void StopTimer();
		virtual bool IsTimerStarted() const;
		virtual size_t GetTimerInterval() const;
		void OnTimer();

	private:
		TWrapper<void, size_t> wrapper;
		size_t interval;
		UINT timerID;
		bool started;
	};
}


#endif // CMAKE_PAINTSNOW

#endif // __ZTIMERDXUT__
