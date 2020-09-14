// TerrainComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../../../SnowyStream/Resource/TerrainResource.h"

namespace PaintsNow {
	class TerrainComponent : public TAllocatedTiny<TerrainComponent, RenderableComponent> {
	public:
		TerrainComponent();
		uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;

		TShared<TerrainResource> terrainResource;
	};
}
