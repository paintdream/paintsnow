#include "ILayout.h"

using namespace PaintsNow;

ILayout::Target::Target() : layout(nullptr), weight(1), start(0), count(-1), remained(0), hide(false), floated(false), fitContent(false), needUpdate(true) {}

void ILayout::Target::UpdateLayout(bool forceUpdate) {
	if (layout != nullptr && (needUpdate || forceUpdate)) {
		needUpdate = false;
		scrollSize = layout->DoLayout(rect, subWindows, start, count, forceUpdate);
		for (std::list<ILayout::Target*>::iterator it = subWindows.begin(); it != subWindows.end(); ++it) {
			// assert(((*it)->rect.first.x() & 0xf0000000) == 0);
			(*it)->needUpdate = true;
		}
	}
}


ILayout::~ILayout() {}
