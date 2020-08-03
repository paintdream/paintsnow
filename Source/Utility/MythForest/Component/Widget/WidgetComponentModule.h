// WidgetComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __WIDGETCOMPONENTMODULE_H__
#define __WIDGETCOMPONENTMODULE_H__

#include "../Batch/BatchComponentModule.h"
#include "WidgetComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class WidgetComponent;
		class WidgetComponentModule  : public TReflected<WidgetComponentModule, ModuleImpl<WidgetComponent> > {
		public:
			WidgetComponentModule(Engine& engine);
			virtual void Initialize();
			virtual void Uninitialize();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<WidgetComponent> RequestNew(IScript::Request& request, IScript::Delegate<BatchComponent> batchComponent, IScript::Delegate<BatchComponent> batchInstancedDataComponent);
			void RequestSetWidgetTexture(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, IScript::Delegate<NsSnowyStream::TextureResource> texture);
			void RequestSetWidgetCoord(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, Float4& inCoord, Float4& outCoord);
			void RequestSetWidgetRepeatMode(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, bool repeatable);

		protected:
			TShared<NsSnowyStream::MeshResource> widgetMesh;
			TShared<NsSnowyStream::MaterialResource> widgetMaterial;
			BatchComponentModule* batchComponentModule;
		};
	}
}


#endif // __WIDGETCOMPONENTMODULE_H__
