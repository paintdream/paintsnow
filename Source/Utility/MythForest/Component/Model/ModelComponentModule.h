// ModelComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __MODELCOMPONENTMODULE_H__
#define __MODELCOMPONENTMODULE_H__

#include "ModelComponent.h"
#include "../Batch/BatchComponentModule.h"
#include "../Renderable/RenderableComponentModule.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ModelComponent;
		class ModelComponentModule : public TReflected<ModelComponentModule, TRenderableComponentModule<ModelComponent> > {
		public:
			ModelComponentModule(Engine& engine, BatchComponentModule& batchComponentModule);

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request, IScript::Delegate<NsSnowyStream::MeshResource> modelResource, IScript::Delegate<BatchComponent> batchComponentHost);
			void RequestAddMaterial(IScript::Request& request, IScript::Delegate<ModelComponent> modelComponent, uint32_t meshGroupIndex, IScript::Delegate<NsSnowyStream::MaterialResource> materialResource);

		protected:
			BatchComponentModule& batchComponentModule;
		};
	}
}


#endif // __MODELCOMPONENTMODULE_H__
