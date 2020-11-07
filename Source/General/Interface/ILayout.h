// ILayout.h
// By PaintDream (paintdream@paintdream.com)
// 2015-1-21
//

#pragma once
#include "../../Core/Interface/IType.h"
#include <list>

namespace PaintsNow {
	class pure_interface ILayout {
	public:
		class Target {
		public:
			Target();
			void UpdateLayout(bool forceUpdate = false);

			std::list<Target*> subWindows;
			ILayout* layout;
			Int2Pair rect; // output
			Int2Pair margin;
			Int2Pair padding;
			Int2Pair size;
			Int2 scrollSize;
			Int2 scrollOffset;
			unsigned short weight;
			int start;
			int count;
			int remained;
			bool floated;
			bool hide;
			bool fitContent;
			bool needUpdate;
		};

		virtual ~ILayout();
		virtual Int2 DoLayout(const Int2Pair& rect, std::list<Target*>& targets, int start, int count, bool forceUpdate) const = 0;
	};
}

