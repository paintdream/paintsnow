// BatchComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "BatchComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class BatchComponentModule : public TReflected<BatchComponentModule, ModuleImpl<BatchComponent> > {
	public:
		BatchComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		TShared<BatchComponent> Create(IRender::Resource::BufferDescription::Usage usage);

	public:
		// APIs
		TShared<BatchComponent> RequestNew(IScript::Request& request, const String& usage);
		void RequestGetCaptureStatistics(IScript::Request& request, IScript::Delegate<BatchComponent> component);
	};
}

