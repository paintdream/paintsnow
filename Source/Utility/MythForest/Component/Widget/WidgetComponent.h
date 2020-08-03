// WidgetComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __WIDGETCOMPONENT_H__
#define __WIDGETCOMPONENT_H__

#include "../../Entity.h"
#include "../Model/ModelComponent.h"

namespace PaintsNow {
	namespace NsMythForest {
		class WidgetComponent : public TAllocatedTiny<WidgetComponent, UniqueComponent<ModelComponent, SLOT_WIDGET_COMPONENT> > {
		public:
			enum {
				WIDGETCOMPONENT_TEXTURE_REPEATABLE = MODELCOMPONENT_CUSTOM_BEGIN,
				WIDGETCOMPONENT_CUSTOM_BEGIN = MODELCOMPONENT_CUSTOM_BEGIN << 1
			};

			WidgetComponent(TShared<NsSnowyStream::MeshResource> meshResource, TShared<BatchComponent> batchUniforms, TShared<BatchComponent> batchInstancedData);
			virtual void GenerateDrawCalls(std::vector<OutputRenderData>& drawCallTemplates, std::vector<std::pair<uint32_t, TShared<NsSnowyStream::MaterialResource> > >& materialResources) override;

			// Custom data
			Float4 inTexCoordRect;
			Float4 outTexCoordRect;
			TShared<NsSnowyStream::TextureResource> mainTexture;
			TShared<BatchComponent> batchInstancedDataComponent;
		};
	}
}


#endif // __WIDGETCOMPONENT_H__
