// Module.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-1
//

#ifndef __VISUALIZER_H__
#define __VISUALIZER_H__

#include "IWidget.h"

namespace PaintsNow {
	namespace NsLeavesWine {
		class Module : public IWidget {
		public:
			virtual void TickRender(NsLeavesFlute::LeavesFlute& leavesFlute) override;
		};
	}
}

#endif // __VISUALIZER_H__
