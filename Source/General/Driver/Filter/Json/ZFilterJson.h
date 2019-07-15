// ZFilterJson.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-10
//

#ifndef __ZFILTERJSON_H__
#define __ZFILTERJSON_H__

#include "../../../../Core/Interface/IFilterBase.h"

namespace PaintsNow {
	class ZFilterJson final : public IFilterBase {
	public:
		virtual IStreamBase* CreateFilter(IStreamBase& streamBase);
	};
}

#endif // __ZFILTERJSON_H__