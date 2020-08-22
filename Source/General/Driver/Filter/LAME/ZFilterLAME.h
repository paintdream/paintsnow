// ZFilterLAME.h
// By PaintDream (paintdream@paintdream.com)
// 2014-12-13
//

#pragma once
#ifndef __linux__
#include <lame.h>
#else
#include <lame/lame.h>
#endif

#include "../../../Interface/IAudio.h"

namespace PaintsNow {
	class ZFilterLAME final : public IFilterBase {
	public:
		virtual IStreamBase* CreateFilter(IStreamBase& inputStream) override;
	};
}

