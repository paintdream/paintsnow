// RenderPort.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-24
//

#pragma once
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../../Core/System/Graph.h"

namespace PaintsNow {
	class Engine;
	class RenderPort : public TReflected<RenderPort, GraphPort<SharedTiny> > {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual void Initialize(IRender& render, IRender::Queue* queue);
		virtual void Uninitialize(IRender& render, IRender::Queue* queue);
		virtual void Commit(std::vector<IRender::Queue*>& fencedQueues, std::vector<IRender::Queue*>& instanceQueues, std::vector<IRender::Queue*>& deletedQueues);
		virtual bool BeginFrame(IRender& render);
		virtual void EndFrame(IRender& render);
		virtual void Tick(Engine& engine, IRender::Queue* queue);
		void UpdateRenderStage();

		std::vector<String> publicSymbols;
		TEvent<Engine&, RenderPort&, IRender::Queue*> eventTickHooks;
	};

	template <class T>
	class TRenderPortReference : public TReflected<TRenderPortReference<T>, RenderPort> {
	public:
		TRenderPortReference() : targetRenderPort(nullptr) {}
		void Initialize(IRender& render, IRender::Queue* mainQueue) override {}
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override {}

		void Tick(Engine& engine, IRender::Queue* queue) override {
			if (!RenderPort::GetLinks().empty()) {
				T* port = RenderPort::GetLinks().back().port->QueryInterface(UniqueType<T>());
				assert(port != nullptr);
				if (port != nullptr) {
					targetRenderPort = port;
				}
			}

			RenderPort::Flag().store(targetRenderPort->Flag().load(std::memory_order_relaxed), std::memory_order_relaxed);
		}

		T* operator -> () {
			return targetRenderPort;
		}

		const T* operator -> () const {
			return targetRenderPort;
		}

		T* targetRenderPort;
	};

	class RenderPortParameterAdapter : public TReflected<RenderPortParameterAdapter, RenderPort> {
	public:
		virtual PassBase::Updater& GetUpdater() = 0;
	};

	template <class T>
	class RenderPortShaderPass : public TReflected<RenderPortShaderPass<T>, RenderPortParameterAdapter> {
	public:
		RenderPortShaderPass(const TShared<ShaderResourceImpl<T> >& s) : shaderResource(s) {}
		void Initialize(IRender& render, IRender::Queue* mainQueue) override {}
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override {}
		PassBase::Updater& GetUpdater() override { return shaderResource->GetPassUpdater(); }

		inline T& GetPass() {
			return static_cast<T&>(shaderResource->GetPass());
		}

	private:
		TShared<ShaderResourceImpl<T> >& shaderResource;
	};
}
