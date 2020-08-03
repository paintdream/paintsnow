// RenderableComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __RENDERABLECOMPONENT_H__
#define __RENDERABLECOMPONENT_H__

#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/GraphicResourceBase.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/SkeletonResource.h"
#include "../RenderFlow/RenderPolicy.h"

namespace PaintsNow {
	namespace NsMythForest {
		class RenderableComponent : public TAllocatedTiny<RenderableComponent, Component>, public NsSnowyStream::IDrawCallProvider {
		public:
			enum {
				RENDERABLECOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN
			};
			
			RenderableComponent();

			virtual Tiny::FLAG GetEntityFlagMask() const override;
			virtual size_t ReportGraphicMemoryUsage() const;

			TShared<RenderPolicy> renderPolicy;
		};
	}
}


#endif // __RENDERABLECOMPONENT_H__
