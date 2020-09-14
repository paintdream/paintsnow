// ZFilterLZW.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-10
//

#pragma once
#include "../../../../Core/Interface/IFilterBase.h"

namespace PaintsNow {
	class ZFilterLZW final : public IFilterBase {
	public:
		IStreamBase* CreateFilter(IStreamBase& streamBase) override;
	};
}

