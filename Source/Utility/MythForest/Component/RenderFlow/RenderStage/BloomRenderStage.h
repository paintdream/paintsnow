// BloomRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __BLOOMRENDERSTAGE_H__
#define __BLOOMRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/BloomPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class BloomRenderStage : public TReflected<BloomRenderStage, GeneralRenderStageRect<NsSnowyStream::BloomPass> > {
		public:
			BloomRenderStage(const String& config);
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput InputColor;
			RenderPortRenderTarget OutputColor;
		};
	}
}

#endif // __BLOOMRENDERSTAGE_H__