// Script.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-1
//

#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include "IWidget.h"

namespace PaintsNow {
	namespace NsLeavesWine {
		class Script : public IWidget {
		public:
			virtual void TickRender(NsLeavesFlute::LeavesFlute& leavesFlute) override;
		};
	}
}

#endif // __SCRIPT_H__