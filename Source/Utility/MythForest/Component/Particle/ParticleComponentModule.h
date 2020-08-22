// ParticleComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "ParticleComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class ParticleComponent;
	class ParticleComponentModule : public TReflected<ParticleComponentModule, ModuleImpl<ParticleComponent> > {
	public:
		ParticleComponentModule(Engine& engine);
		virtual ~ParticleComponentModule();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ParticleComponent> RequestNew(IScript::Request& request);
		void RequestRebuild(IScript::Request& request, IScript::Delegate<ParticleComponent> particleComponent);
	};
}

