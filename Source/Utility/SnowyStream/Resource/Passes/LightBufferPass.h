// LightBufferPass.h
// LightBufferFS Pass
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __LIGHTPASS_PASS_H__
#define __LIGHTPASS_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/LightEncoderFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class LightBufferPass : public TReflected<LightBufferPass, ZPassBase> {
		public:
			LightBufferPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			// Vertex shaders
			ScreenTransformVS transform;

			// Fragment shaders
			LightEncoderFS encoder;
		};
	}
}


#endif // __LIGHTPASS_PASS_H__
