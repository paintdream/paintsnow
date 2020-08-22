// EnvCubeComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "EnvCubeComponent.h"
#include "../Renderable/RenderableComponentModule.h"

namespace PaintsNow {
	class Entity;
	class EnvCubeComponent;
	class EnvCubeComponentModule : public TReflected<EnvCubeComponentModule, TRenderableComponentModule<EnvCubeComponent> > {
	public:
		EnvCubeComponentModule(Engine& engine);
		virtual ~EnvCubeComponentModule();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<EnvCubeComponent> RequestNew(IScript::Request& request);
		void RequestSetTexture(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, IScript::Delegate<TextureResource> textureResource);
		void RequestSetRange(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, Float3& range);
	};
}

