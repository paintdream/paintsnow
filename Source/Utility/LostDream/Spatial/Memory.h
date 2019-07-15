// Memory.h
// PaintDream (paintdream@paintdream.com)
// 2019-10-11
//

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "../LostDream.h"

namespace PaintsNow {
	namespace NsLostDream {
		class Memory : public TReflected<Memory, LostDream::Qualifier> {
		public:
			virtual bool Initialize();
			virtual bool Run(int randomSeed, int length);
			virtual void Summary();

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		};
	}
}

#endif // __MEMORY_H__