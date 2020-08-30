// StandardRenderFlow.h
// By PaintDream (paintdream@paintdream.com)
// 2018-8-12
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/System/Graph.h"
#include "RenderStage.h"

namespace PaintsNow {
	class RenderFlowComponent : public TAllocatedTiny<RenderFlowComponent, Component>, protected Graph<RenderStage> {
	public:
		RenderFlowComponent();
		virtual ~RenderFlowComponent();

		enum {
			RENDERFLOWCOMPONENT_SYNC_DEVICE_RESOLUTION = COMPONENT_CUSTOM_BEGIN,
			RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED = COMPONENT_CUSTOM_BEGIN << 1,
			RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING = COMPONENT_CUSTOM_BEGIN << 2,
			RENDERFLOWCOMPONENT_RENDERING = COMPONENT_CUSTOM_BEGIN << 3,
			RENDERFLOWCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 4
		};

		virtual void Initialize(Engine& engine, Entity* entity) override;
		virtual void Uninitialize(Engine& engine, Entity* entity) override;
		virtual void DispatchEvent(Event& event, Entity* entity) override;
		virtual Tiny::FLAG GetEntityFlagMask() const override;

		void AddNode(RenderStage* renderStage);
		void RemoveNode(RenderStage* renderStage);
		// set 0 for screen size
		const Int2& GetMainResolution() const;
		void SetMainResolution(const Int2& res);

		RenderStage::Port* BeginPort(const String& symbol);
		void EndPort(RenderStage::Port* port);
		bool ExportSymbol(const String& symbol, RenderStage* renderStage, const String& port);
		void Compile();
		void Optimize(bool enableParallelPresent);
		void Render(Engine& engine);
		void RenderSyncTick(Engine& engine);

	protected:
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		void SetMainResolution(Engine& engine);

		Int2 mainResolution;
		std::map<String, std::pair<RenderStage*, String> > symbolMap;
		std::vector<RenderStage*> cachedRenderStages;
		IRender::Queue* resourceQueue;
	};
}

