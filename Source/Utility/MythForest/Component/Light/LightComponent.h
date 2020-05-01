// LightComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __LIGHTCOMPONENT_H__
#define __LIGHTCOMPONENT_H__

#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../Stream/StreamComponent.h"

namespace PaintsNow {
	namespace NsMythForest {
		class LightComponent : public TAllocatedTiny<LightComponent, RenderableComponent> {
		public:
			LightComponent();
			enum {
				LIGHTCOMPONENT_DIRECTIONAL = COMPONENT_CUSTOM_BEGIN,
				LIGHTCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
			};

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual FLAG GetEntityFlagMask() const override;
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData);
			virtual void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
			void BindShadowStream(uint32_t layer, TShared<StreamComponent> streamComponent);

			Float3 color;
			float attenuation;
			Float3 lightRange;
			// float spotAngle;
			// float temperature;
		protected:
			std::vector<TShared<StreamComponent> > shadowLayers;
		};
	}
}


#endif // __LIGHTCOMPONENT_H__
