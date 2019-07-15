// MultiHashLight.h
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __MULTIHASHLIGHT_PASS_H__
#define __MULTIHASHLIGHT_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/MultiHashLightFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MultiHashLightPass : public TReflected<MultiHashLightPass, ZPassBase> {
		public:
			MultiHashLightPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			ScreenTransformVS screenTransform;
			MultiHashLightFS shaderMultiHashLight;
		};
	}
}


#endif // __MULTIHASHLIGHT_PASS_H__
