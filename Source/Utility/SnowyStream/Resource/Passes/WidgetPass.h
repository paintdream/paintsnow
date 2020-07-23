// WidgetPass.h
// Widget Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __WIDGET_PASS_H__
#define __WIDGET_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/WidgetTransformVS.h"
#include "../Shaders/WidgetShadingFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
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
}


#endif // __WIDGET_PASS_H__
