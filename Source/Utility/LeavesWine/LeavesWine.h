// LeavesWine.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-3
//

#ifndef __LEAVESWINE_H__
#define __LEAVESWINE_H__

#include "IWidget.h"
#include <vector>

namespace PaintsNow {
	namespace NsLeavesWine {
		class LeavesWine {
		public:
			LeavesWine(NsLeavesFlute::LeavesFlute*& leavesFlute);
			void AddWidget(IWidget* widget);
			void TickRender();

		protected:
			std::vector<IWidget*> widgets;
			NsLeavesFlute::LeavesFlute*& leavesFlute;
		};
	}
}

#endif // __LEAVESWINE_H__