// DepthResolvePass.h
// DepthResolveFS Pass
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __DEPTHRESOLVE_PASS_H__
#define __DEPTHRESOLVE_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DepthResolveFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class DepthResolvePass : public TReflected<DepthResolvePass, PassBase> {
		public:
			DepthResolvePass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			// Vertex shaders
			ScreenTransformVS transform;

			// Fragment shaders
			DepthResolveFS resolve;
		};
	}
}


#endif // __DEPTHRESOLVE_PASS_H__
