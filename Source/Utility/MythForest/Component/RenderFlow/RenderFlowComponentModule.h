// RenderFlowComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __RENDERFLOWCOMPONENTMODULE_H__
#define __RENDERFLOWCOMPONENTMODULE_H__

#include "RenderFlowComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class RenderStage;
		class RenderFlowComponent;
		class RenderFlowComponentModule  : public TReflected<RenderFlowComponentModule , ModuleImpl<RenderFlowComponent> > {
		public:
			RenderFlowComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void RegisterNodeTemplate(String& key, const TFactoryBase<RenderStage>& t);

			void RequestNew(IScript::Request& request);
			void RequestNewRenderPolicy(IScript::Request& request, const String& name, uint32_t priority);
			void RequestNewRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlow, const String& name, const String& config);
			void RequestEnumerateRenderStagePorts(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> renderStage);
			void RequestLinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName);
			void RequestUnlinkRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> from, const String& fromPortName, IScript::Delegate<RenderStage> to, const String& toPortName);
			void RequestExportRenderStagePort(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, IScript::Delegate<RenderStage> stage, const String& portName, const String& symbol);
			void RequestBindRenderTargetTexture(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& symbol, IScript::Delegate<NsSnowyStream::TextureResource> renderTargetResource);
			void RequestDeleteRenderStage(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlow, IScript::Delegate<RenderStage> renderStage);

		protected:
			std::map<String, TFactoryBase<RenderStage> > stageTemplates;
		};
	}
}


#endif // __RENDERFLOWCOMPONENTMODULE_H__
