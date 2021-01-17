// WidgetComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Model/ModelComponent.h"

namespace PaintsNow {
	class WidgetComponent : public TAllocatedTiny<WidgetComponent, ModelComponent> {
	public:
		enum {
			WIDGETCOMPONENT_TEXTURE_REPEATABLE = MODELCOMPONENT_CUSTOM_BEGIN,
			WIDGETCOMPONENT_CUSTOM_BEGIN = MODELCOMPONENT_CUSTOM_BEGIN << 1
		};

		WidgetComponent(const TShared<MeshResource>& meshResource, const TShared<BatchComponent>& batchUniforms, const TShared<BatchComponent>& batchInstancedData);
		void GenerateDrawCalls(std::vector<OutputRenderData>& drawCallTemplates, std::vector<std::pair<uint32_t, TShared<MaterialResource> > >& materialResources) override;

		// Custom data
		Float4 inTexCoordRect;
		Float4 outTexCoordRect;
		TShared<TextureResource> mainTexture;
		TShared<BatchComponent> batchInstancedDataComponent;
	};
}
