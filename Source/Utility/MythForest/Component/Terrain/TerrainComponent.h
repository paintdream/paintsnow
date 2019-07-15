// TerrainComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __TERRAINCOMPONENT_H__
#define __TERRAINCOMPONENT_H__

#include "../../Entity.h"
#include "../Renderable/RenderableComponent.h"
#include "../../../SnowyStream/Resource/TerrainResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		class TerrainComponent : public TAllocatedTiny<TerrainComponent, RenderableComponent> {
		public:
			TerrainComponent();
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;

			TShared<NsSnowyStream::TerrainResource> terrainResource;
		};
	}
}


#endif // __TERRAINCOMPONENT_H__
