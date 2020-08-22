// EnvCubeComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../Renderable/RenderableComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/TextureResource.h"

namespace PaintsNow {
	class EnvCubeComponent : public TAllocatedTiny<EnvCubeComponent, RenderableComponent> {
	public:
		EnvCubeComponent();
		virtual ~EnvCubeComponent();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual FLAG GetEntityFlagMask() const override;
		virtual void UpdateBoundingBox(Engine& engine, Float3Pair& box) override;
		virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData);

		TShared<TextureResource> cubeMapTexture; // pre-filterred specular texture
		TShared<TextureResource> skyMapTexture; // irrandiance map texture
		Float3 range;
	};
}

