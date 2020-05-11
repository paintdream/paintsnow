// LightComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __LIGHTCOMPONENTMODULE_H__
#define __LIGHTCOMPONENTMODULE_H__

#include "LightComponent.h"
#include "../Renderable/RenderableComponentModule.h"

namespace PaintsNow {
	namespace NsMythForest {
		class LightComponent;
		class LightComponentModule : public TReflected<LightComponentModule, TRenderableComponentModule<LightComponent> > {
		public:
			LightComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestSetLightDirectional(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, bool directional);
			void RequestSetLightColor(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, Float3& color);
			void RequestSetLightAttenuation(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, float attenuation);
			void RequestSetLightRange(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, Float3& range);
			void RequestBindLightShadowStream(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, uint32_t layer, IScript::Delegate<StreamComponent> streamComponent, const UShort2& resolution, float gridSize, float scale);
			// void RequestSetLightSpotAngle(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, float spotAngle);
		};
	}
}


#endif // __LIGHTCOMPONENTMODULE_H__
