// DepthBoundingPass.h
// DepthBoundingFS Pass
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __DEPTHBOUNDING_PASS_H__
#define __DEPTHBOUNDING_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DepthMinMaxFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class DepthBoundingPass : public TReflected<DepthBoundingPass, ZPassBase> {
		public:
			DepthBoundingPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			// Vertex shaders
			ScreenTransformVS transform;

			// Fragment shaders
			DepthMinMaxFS minmax;
		};
	}
}


#endif // __DEPTHBOUNDING_PASS_H__
