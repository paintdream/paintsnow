// MultiHashGather.h
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __MULTIHASHGATHER_PASS_H__
#define __MULTIHASHGATHER_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/MultiHashGatherFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MultiHashGatherPass : public TReflected<MultiHashGatherPass, PassBase> {
		public:
			MultiHashGatherPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			ScreenTransformVS screenTransform;
			MultiHashGatherFS shaderMultiHashGather;
		};
	}
}


#endif // __MULTIHASHGATHER_PASS_H__
