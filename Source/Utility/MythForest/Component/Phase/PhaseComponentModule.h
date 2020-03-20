// PhaseComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __PHASECOMPONENTMODULE_H__
#define __PHASECOMPONENTMODULE_H__

#include "PhaseComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class PhaseComponent;
		class PhaseComponentModule  : public TReflected<PhaseComponentModule , ModuleImpl<PhaseComponent> > {
		public:
			PhaseComponentModule(Engine& engine);
			virtual ~PhaseComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& portName);
			void RequestSetup(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t phaseCount, uint32_t taskCount, const Float3& range, const UShort2& resolution);
			void RequestUpdate(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, const Float3& center);
			void RequestStep(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t bounceCount);
			void RequestResample(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t phaseCount);
			void RequestBindRootEntity(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, IScript::Delegate<Entity> rootEntity);
		};
	}
}


#endif // __PHASECOMPONENTMODULE_H__