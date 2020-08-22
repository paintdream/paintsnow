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

	class RenderPortRenderTarget : public TReflected<RenderPortRenderTarget, RenderPort> {
	public:
		RenderPortRenderTarget(IRender::Resource::RenderTargetDescription::Storage& storage);
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual bool UpdateDataStream(RenderPort& source) override;

		TShared<TextureResource> renderTargetTextureResource;
		IRender::Resource::RenderTargetDescription::Storage& bindingStorage;
	};
}

