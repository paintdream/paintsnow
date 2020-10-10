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

		/// <summary>
		/// Create ModelComponent
		/// </summary>
		/// <param name="meshResource"> MeshResource object </param>
		/// <param name="batchComponentHost"> BatchComponent object </param>
		/// <returns> ModelComponent object </returns>
		TShared<ModelComponent> RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> meshResource, IScript::Delegate<BatchComponent> batchComponentHost);

		/// <summary>
		/// Set Material for ModelComponent
		/// </summary>
		/// <param name="modelComponent"> the ModelComponent </param>
		/// <param name="meshGroupIndex"> index of mesh group </param>
		/// <param name="materialResource"> the MaterialResource </param>
		void RequestSetMaterial(IScript::Request& request, IScript::Delegate<ModelComponent> modelComponent, uint32_t meshGroupIndex, IScript::Delegate<MaterialResource> materialResource);

	protected:
		BatchComponentModule* batchComponentModule;
	};
}
