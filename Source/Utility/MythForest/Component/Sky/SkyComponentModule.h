// SkyComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "SkyComponent.h"
#include "../Batch/BatchComponentModule.h"
#include "../Renderable/RenderableComponentModule.h"
#include "../../Module.h"

namespace PaintsNow {
	class SkyComponent;
	class SkyComponentModule : public TReflected<SkyComponentModule, TRenderableComponentModule<SkyComponent> > {
	public:
		SkyComponentModule(Engine& engine);

		void Initialize() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create SkyComponent
		/// </summary>
		/// <param name="meshResource"> MeshResource object </param>
		/// <param name="batchComponentHost"> BatchComponent object </param>
		/// <returns> SkyComponent object </returns>
		TShared<SkyComponent> RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> meshResource, IScript::Delegate<MaterialResource> materialResource, IScript::Delegate<BatchComponent> batchComponentHost);

	protected:
		BatchComponentModule* batchComponentModule;
		TShared<MaterialResource> defaultSkyMaterial;
		TShared<MeshResource> defaultSkyMesh;
	};
}
