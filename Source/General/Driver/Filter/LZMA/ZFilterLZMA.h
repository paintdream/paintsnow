#ifndef __ZFILTERLZMA_H__
#define __ZFILTERLZMA_H__

#include "../../../../Core/Interface/IFilterBase.h"

namespace PaintsNow {
	class ZFilterLZMA final : public IFilterBase {
	public:
		virtual IStreamBase* CreateFilter(IStreamBase& streamBase);
	};
}


#endif // __ZFILTERLZMA_H__
