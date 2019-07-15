// ZFilterBPTC.h
// By PaintDream (paintdream@paintdream.com)
// 2019-7-29
//

#ifndef __ZFILTERBPTC_H__
#define __ZFILTERBPTC_H__

#include "../../../../Core/Interface/IFilterBase.h"

namespace PaintsNow {
	class ZFilterBPTC final : public IFilterBase {
	public:
		virtual IStreamBase* CreateFilter(IStreamBase& streamBase);
	};
}

#endif // __ZFILTERBPTC_H__