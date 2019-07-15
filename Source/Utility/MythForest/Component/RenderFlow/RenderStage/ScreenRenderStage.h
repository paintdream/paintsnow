// ScreenRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __SCREENRENDERSTAGE_H__
#define __SCREENRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/ScreenPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ScreenRenderStage : public TReflected<ScreenRenderStage, GeneralRenderStageRect<NsSnowyStream::ScreenPass> > {
		public:
			ScreenRenderStage(const String& config = "1");
			virtual void PrepareResources(Engine& engine);
			virtual void UpdatePass(Engine& engine);

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput InputColor;
			RenderPortRenderTarget OutputColor;

			std::vector<TShared<RenderPortTextureInput> > BloomLayers;
		};
	}
}

#endif // __SCREENRENDERSTAGE_H__