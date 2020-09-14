// LightComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "LightComponent.h"
#include "../Renderable/RenderableComponentModule.h"

namespace PaintsNow {
	class LightComponent;
	class LightComponentModule : public TReflected<LightComponentModule, TRenderableComponentModule<LightComponent> > {
	public:
		LightComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<LightComponent> RequestNew(IScript::Request& request);
		void RequestSetLightDirectional(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, bool directional);
		void RequestSetLightColor(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, Float3& color);
		void RequestSetLightAttenuation(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, float attenuation);
		void RequestSetLightRange(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, Float3& range);
		void RequestBindLightShadowStream(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, uint32_t layer, IScript::Delegate<StreamComponent> streamComponent, const UShort2& resolution, float gridSize, float scale);
		// void RequestSetLightSpotAngle(IScript::Request& request, IScript::Delegate<LightComponent> lightComponent, float spotAngle);
	};
}
