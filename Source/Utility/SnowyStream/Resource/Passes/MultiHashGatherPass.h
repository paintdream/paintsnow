// MultiHashGather.h
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/MultiHashGatherFS.h"

namespace PaintsNow {
	class MultiHashGatherPass : public TReflected<MultiHashGatherPass, PassBase> {
	public:
		MultiHashGatherPass();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		ScreenTransformVS screenTransform;
		MultiHashGatherFS shaderMultiHashGather;
	};
}
