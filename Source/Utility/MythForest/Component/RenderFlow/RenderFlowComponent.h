// StandardRenderFlow.h
// PaintDream (paintdream@paintdream.com)
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
		~RenderFlowComponent() override;

		enum {
			RENDERFLOWCOMPONENT_SYNC_DEVICE_RESOLUTION = COMPONENT_CUSTOM_BEGIN,
			RENDERFLOWCOMPONENT_RESOLUTION_MODIFIED = COMPONENT_CUSTOM_BEGIN << 1,
			RENDERFLOWCOMPONENT_RENDER_SYNC_TICKING = COMPONENT_CUSTOM_BEGIN << 2,
			RENDERFLOWCOMPONENT_RENDERING = COMPONENT_CUSTOM_BEGIN << 3,
			RENDERFLOWCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 4
		};

		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		void DispatchEvent(Event& event, Entity* entity) override;
		Tiny::FLAG GetEntityFlagMask() const override;

		void AddNode(RenderStage* renderStage);
		void RemoveNode(RenderStage* renderStage);
		// set 0 for screen size
		UShort2 GetMainResolution() const;
		void SetMainResolution(const UShort2 res);
		void ResolveSamplessAttachments();
		void SetupTextures(Engine& engine);

		RenderStage::Port* BeginPort(const String& symbol);
		void EndPort(RenderStage::Port* port);
		bool ExportSymbol(const String& symbol, RenderStage* renderStage, const String& port);
		void Compile();
		void Render(Engine& engine);
		void RenderSyncTick(Engine& engine);

	protected:
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void SetMainResolution(Engine& engine);

		UShort2 mainResolution;
		std::map<String, std::pair<RenderStage*, String> > symbolMap;
		std::vector<RenderStage*> cachedRenderStages;
		IRender::Queue* resourceQueue;
		IThread::Lock* frameSyncLock;
		IThread::Event* frameSyncEvent;
		IRender::Resource* eventResourcePrepared;
	};
}

