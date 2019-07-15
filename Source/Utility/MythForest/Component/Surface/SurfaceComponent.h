// SurfaceComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __SURFACECOMPONENT_H__
#define __SURFACECOMPONENT_H__

#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"

namespace PaintsNow {
	namespace NsMythForest {
		class SurfaceComponent : public TAllocatedTiny<SurfaceComponent, RenderableComponent> {
		public:
			SurfaceComponent();
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;
		};
	}
}


#endif // __SURFACECOMPONENT_H__
