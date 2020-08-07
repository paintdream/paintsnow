// RenderFlowComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __RENDERFLOWCOMPONENTMODULE_H__
#define __RENDERFLOWCOMPONENTMODULE_H__

#include "RenderFlowComponent.h"
#include "RenderPolicy.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class RenderStage;
		class RenderFlowComponent;
		class RenderFlowComponentModule  : public TReflected<RenderFlowComponentModule, ModuleImpl<RenderFlowComponent> > {
		public:
			RenderFlowComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void RegisterNodeTemplate(String& key, const TWrapper<RenderStage*, const String&>& t);

			TShared<RenderFlowComponent> RequestNew(IScript::Request& request);
			TShared<RenderPolicy> RequestNewRenderPolicy(IScript::Request& request, const String& name, uint32_t priority);
			TShared<RenderStage> RequestNewRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlow, const String& name, const String& config);
			void RequestEnumerateRenderStagePorts(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> renderStage);
			void RequestLinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName);
			void RequestUnlinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName);
			void RequestExportRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> stage, const String& portName, const String& symbol);
			void RequestBindRenderTargetTexture(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& symbol, IScript::Delegate<NsSnowyStream::TextureResource> renderTargetResource);
			void RequestDeleteRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlow, IScript::Delegate<RenderStage> renderStage);

		protected:
			std::map<String, TWrapper<RenderStage*, const String&> > stageTemplates;
		};
	}
}


#endif // __RENDERFLOWCOMPONENTMODULE_H__
