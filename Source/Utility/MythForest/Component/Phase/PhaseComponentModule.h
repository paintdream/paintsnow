// PhaseComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "PhaseComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class PhaseComponent;
	class PhaseComponentModule : public TReflected<PhaseComponentModule, ModuleImpl<PhaseComponent> > {
	public:
		PhaseComponentModule(Engine& engine);
		~PhaseComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<PhaseComponent> RequestNew(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& portName);
		void RequestSetup(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t phaseCount, uint32_t taskCount, const Float3& range, const UShort2& resolution);
		void RequestUpdate(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, const Float3& center);
		void RequestStep(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t bounceCount);
		void RequestResample(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t phaseCount);
		void RequestBindRootEntity(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, IScript::Delegate<Entity> rootEntity);
		void RequestSetDebugMode(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, const String& debugPath);
	};
}

