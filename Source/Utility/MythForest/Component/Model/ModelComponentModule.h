// ModelComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "ModelComponent.h"
#include "../Batch/BatchComponentModule.h"
#include "../Renderable/RenderableComponentModule.h"
#include "../../Module.h"

namespace PaintsNow {
	class ModelComponent;
	class ModelComponentModule : public TReflected<ModelComponentModule, TRenderableComponentModule<ModelComponent> > {
	public:
		ModelComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ModelComponent> RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> modelResource, IScript::Delegate<BatchComponent> batchComponentHost);
		void RequestAddMaterial(IScript::Request& request, IScript::Delegate<ModelComponent> modelComponent, uint32_t meshGroupIndex, IScript::Delegate<MaterialResource> materialResource);

	protected:
		BatchComponentModule* batchComponentModule;
	};
}
