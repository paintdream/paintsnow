// ShadowMaskPass.h
// ShadowMaskFS Pass
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __SHADOWMASK_PASS_H__
#define __SHADOWMASK_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/ShadowMaskFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ShadowMaskPass : public TReflected<ShadowMaskPass, PassBase> {
		public:
			ShadowMaskPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			// Vertex shaders
			ScreenTransformVS transform;

			// Fragment shaders
			ShadowMaskFS mask;
		};
	}
}


#endif // __SHADOWMASK_PASS_H__
