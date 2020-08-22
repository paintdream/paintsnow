// LeavesWine.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-3
//

#pragma once
#include "IWidget.h"
#include <vector>

namespace PaintsNow {
	class LeavesWine {
	public:
		LeavesWine(LeavesFlute*& leavesFlute);
		void AddWidget(IWidget* widget);
		void TickRender();

	protected:
		std::vector<IWidget*> widgets;
		LeavesFlute*& leavesFlute;
	};
}

