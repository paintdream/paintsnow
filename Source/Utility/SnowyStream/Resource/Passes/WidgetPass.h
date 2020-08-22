// WidgetPass.h
// Widget Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/WidgetTransformVS.h"
#include "../Shaders/WidgetShadingFS.h"

namespace PaintsNow {
	class WidgetPass : public TReflected<WidgetPass, PassBase> {
	public:
		WidgetPass();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		WidgetTransformVS widgetTransform;

		// Fragment shaders
		WidgetShadingFS widgetShading;
	};
}
