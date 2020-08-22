// RenderPortLoadTarget.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"

namespace PaintsNow {
	class RenderPortLoadTarget : public TReflected<RenderPortLoadTarget, RenderPort> {
	public:
		RenderPortLoadTarget(IRender::Resource::RenderTargetDescription::Storage& storage, bool save = false);
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual bool UpdateDataStream(RenderPort& source) override;

		IRender::Resource::RenderTargetDescription::Storage& bindingStorage;
	};
}

