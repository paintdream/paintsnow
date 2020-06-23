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
			WidgetComponent(NsSnowyStream::MeshResource& quadMesh);
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;

			Float4 inTexCoordRect;
			Float4 outTexCoordRect;
			NsSnowyStream::MeshResource& quadMesh;
			TShared<NsSnowyStream::TextureResource> mainTexture;
			TShared<NsSnowyStream::MaterialResource> materialResource;
		};
	}
}


#endif // __WIDGETCOMPONENT_H__
