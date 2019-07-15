// EnvCubeComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __ENVCUBECOMPONENTMODULE_H__
#define __ENVCUBECOMPONENTMODULE_H__

#include "EnvCubeComponent.h"
#include "../Renderable/RenderableComponentModule.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class EnvCubeComponent;
		class EnvCubeComponentModule : public TReflected<EnvCubeComponentModule, TRenderableComponentModule<EnvCubeComponent> > {
		public:
			EnvCubeComponentModule(Engine& engine);
			virtual ~EnvCubeComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestSetTexture(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, IScript::Delegate<NsSnowyStream::TextureResource> textureResource);
			void RequestSetRange(IScript::Request& request, IScript::Delegate<EnvCubeComponent> envCubeComponent, Float3& range);
		};
	}
}


#endif // __ENVCUBECOMPONENTMODULE_H__