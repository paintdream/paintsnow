// RenderPortSharedBuffer.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "RenderPortRenderTarget.h"

namespace PaintsNow {
	class RenderPortSharedBufferStore;
	class RenderPortSharedBufferLoad : public TReflected<RenderPortSharedBufferLoad, RenderPort> {
	public:
		RenderPortSharedBufferLoad(bool save = false);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		void Tick(Engine& engine, IRender::Queue* queue) override;

		IRender::Resource* sharedBufferResource;
	};

	class RenderPortSharedBufferStore : public TReflected<RenderPortSharedBufferStore, RenderPort> {
	public:
		RenderPortSharedBufferStore();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		void Tick(Engine& engine, IRender::Queue* queue) override;

		IRender::Resource* sharedBufferResource;
	};
}

