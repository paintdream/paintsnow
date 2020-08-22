// MultiHashLight.h
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/MultiHashLightFS.h"

namespace PaintsNow {
	class MultiHashLightPass : public TReflected<MultiHashLightPass, PassBase> {
	public:
		MultiHashLightPass();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		ScreenTransformVS screenTransform;
		MultiHashLightFS shaderMultiHashLight;
	};
}
