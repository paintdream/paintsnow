// ScreenSpaceTraceRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"

namespace PaintsNow {
	class ScreenSpaceTraceRenderStage : public TReflected<ScreenSpaceTraceRenderStage, RenderStage> {
	public:
		ScreenSpaceTraceRenderStage(const String& s);

		RenderPortTextureInput Depth;
	};
}

