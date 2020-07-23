// AntiAliasingPass.h
// AntiAliasingFS Pass
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __ANTIALIASING_PASS_H__
#define __ANTIALIASING_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/AntiAliasingFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class AntiAliasingPass : public TReflected<AntiAliasingPass, PassBase> {
		public:
			AntiAliasingPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			// Vertex shaders
			ScreenTransformVS transform;

			// Fragment shaders
			AntiAliasingFS antiAliasing;
		};
	}
}


#endif // __ANTIALIASING_PASS_H__
