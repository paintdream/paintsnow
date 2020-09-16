// RenderPortRenderTarget.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"

namespace PaintsNow {
	class MetaAdaptMainResolution : public MetaNodeBase {
	public:
		MetaAdaptMainResolution(int8_t bitwiseShift = 0);
		MetaAdaptMainResolution operator = (int8_t bitwiseShift);

		template <class T, class D>
		inline const MetaAdaptMainResolution& FilterField(T* t, D* d) const {
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaAdaptMainResolution Type;
		};

		typedef MetaAdaptMainResolution Type;


		uint8_t bitwiseShift;
	};

	extern MetaAdaptMainResolution AdaptMainResolution;

	class RenderPortRenderTargetLoad : public TReflected<RenderPortRenderTargetLoad, RenderPort> {
	public:
		RenderPortRenderTargetLoad(IRender::Resource::RenderTargetDescription::Storage& storage, bool save = false);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		bool UpdateDataStream(RenderPort& source) override;

		IRender::Resource::RenderTargetDescription::Storage& bindingStorage;
	};

	class RenderPortRenderTargetStore : public TReflected<RenderPortRenderTargetStore, RenderPort> {
	public:
		RenderPortRenderTargetStore(IRender::Resource::RenderTargetDescription::Storage& storage);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		bool UpdateDataStream(RenderPort& source) override;

		TShared<TextureResource> renderTargetTextureResource;
		IRender::Resource::RenderTargetDescription::Storage& bindingStorage;
	};
}

