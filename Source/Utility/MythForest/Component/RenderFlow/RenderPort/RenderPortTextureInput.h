// RenderPortTextureInput.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#ifndef __RENDERPORTTEXTUREINPUT_H__
#define __RENDERPORTTEXTUREINPUT_H__

#include "../RenderPort.h"

namespace PaintsNow {
	namespace NsMythForest {
		class RenderStage;
		class RenderPortTextureInput : public TReflected<RenderPortTextureInput, RenderPort> {
		public:
			RenderPortTextureInput();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			TShared<NsSnowyStream::TextureResource> textureResource;
			RenderStage* linkedRenderStage;

			virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override;
			virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
			virtual bool UpdateDataStream(RenderPort& source) override;
		};
	}
}

#endif // __RENDERPORTTEXTUREINPUT_H__