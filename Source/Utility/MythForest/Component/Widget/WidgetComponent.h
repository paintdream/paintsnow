// WidgetComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __WIDGETCOMPONENT_H__
#define __WIDGETCOMPONENT_H__

#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		class WidgetComponent : public TAllocatedTiny<WidgetComponent, UniqueComponent<RenderableComponent, SLOT_WIDGET_COMPONENT> > {
		public:
			enum {
				WIDGETCOMPONENT_TEXTURE_REPEATABLE = RENDERABLECOMPONENT_CUSTOM_BEGIN,
				WIDGETCOMPONENT_CUSTOM_BEGIN = RENDERABLECOMPONENT_CUSTOM_BEGIN << 1
			};

			WidgetComponent(NsSnowyStream::MeshResource& quadMesh, TShared<NsSnowyStream::MaterialResource> material);
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;

			Float4 inTexCoordRect;
			Float4 outTexCoordRect;
			NsSnowyStream::MeshResource& quadMesh;
			TShared<NsSnowyStream::TextureResource> mainTexture;
			TShared<NsSnowyStream::MaterialResource> materialResource;
			uint32_t mainTextureSlotInMaterial;
		};
	}
}


#endif // __WIDGETCOMPONENT_H__
