// RenderPortTextureInput.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"

namespace PaintsNow {
	class RenderStage;
	class RenderPortTextureInput : public TReflected<RenderPortTextureInput, RenderPort> {
	public:
		RenderPortTextureInput();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		TShared<TextureResource> textureResource;
		RenderStage* linkedRenderStage;

		virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual bool UpdateDataStream(RenderPort& source) override;
	};
}

