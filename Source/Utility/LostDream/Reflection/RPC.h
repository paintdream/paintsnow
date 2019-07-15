// RPC.h
// By PaintDream (paintdream@paintdream.com)
// 2018-2-10
//

#ifndef __RPCT_H__
#define __RPCT_H__

#include "../LostDream.h"

namespace PaintsNow {
	namespace NsLostDream {
		class RPC : public TReflected<RPC, LostDream::Qualifier> {
		public:
			virtual bool Initialize();
			virtual bool Run(int randomSeed, int length);
			virtual void Summary();

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		};
	}
}

#endif // __RPCT_H__
