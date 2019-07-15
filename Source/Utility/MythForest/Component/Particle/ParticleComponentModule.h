// ParticleComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __PARTICLECOMPONENTMODULE_H__
#define __PARTICLECOMPONENTMODULE_H__

#include "ParticleComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class ParticleComponent;
		class ParticleComponentModule  : public TReflected<ParticleComponentModule , ModuleImpl<ParticleComponent> > {
		public:
			ParticleComponentModule(Engine& engine);
			virtual ~ParticleComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestRebuild(IScript::Request& request, IScript::Delegate<ParticleComponent> particleComponent);
		};
	}
}


#endif // __PARTICLECOMPONENTMODULE_H__