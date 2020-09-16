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
		TObject<IReflect>& operator () (IReflect& reflect) override;
		TShared<TextureResource> textureResource;

		void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
	};
}

