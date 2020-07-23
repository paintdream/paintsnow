// RenderPort.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-24
//

#ifndef __RENDERPORT_H__
#define __RENDERPORT_H__

#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../../Core/System/Graph.h"

namespace PaintsNow {
	class FencedRenderQueue;
	namespace NsMythForest {
		class Engine;
		class RenderPort : public TReflected<RenderPort, GraphPort<SharedTiny> > {
		public:
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void Initialize(IRender& render, IRender::Queue* queue);
			virtual void Uninitialize(IRender& render, IRender::Queue* queue);
			virtual bool UpdateDataStream(RenderPort& source);
			virtual void Commit(std::vector<FencedRenderQueue*>& queues);
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
			virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override {}
			virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override {}
			virtual void Tick(Engine& engine, IRender::Queue* queue) override {
				RenderPort::Flag().store(targetRenderPort->Flag().load(std::memory_order_relaxed), std::memory_order_relaxed);
			}

			virtual bool UpdateDataStream(RenderPort& source) override {
				T* port = source.QueryInterface(UniqueType<T>());
				assert(port != nullptr);
				if (port != nullptr) {
					targetRenderPort = port;
					return true;
				} else {
					return false;
				}
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
			RenderPortShaderPass(TShared<NsSnowyStream::ShaderResourceImpl<T> >& s) : shaderResource(s) {}
			virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override {}
			virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override {}
			virtual bool UpdateDataStream(RenderPort& source) override { return true; }
			virtual PassBase::Updater& GetUpdater() override { return shaderResource->GetPassUpdater(); }

			inline T& GetPass() {
				return static_cast<T&>(shaderResource->GetPass());
			}

		private:
			TShared<NsSnowyStream::ShaderResourceImpl<T> >& shaderResource;
		};
	}
}


#endif // __RENDERPORT_H__
