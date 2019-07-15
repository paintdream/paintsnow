// DepthBoundingSetupPass.h
// DepthBoundingSetupFS Pass
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __DEPTHBOUNDINGSETUP_PASS_H__
#define __DEPTHBOUNDINGSETUP_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DepthMinMaxSetupFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class DepthBoundingSetupPass : public TReflected<DepthBoundingSetupPass, ZPassBase> {
		public:
			DepthBoundingSetupPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			// Vertex shaders
			ScreenTransformVS transform;

			// Fragment shaders
			DepthMinMaxSetupFS minmax;
		};
	}
}


#endif // __DEPTHBOUNDINGSETUP_PASS_H__
