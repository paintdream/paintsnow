// RenderableComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/GraphicResourceBase.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/SkeletonResource.h"
#include "../RenderFlow/RenderPolicy.h"

namespace PaintsNow {
	class RenderableComponent : public TAllocatedTiny<RenderableComponent, Component>, public IDrawCallProvider {
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
