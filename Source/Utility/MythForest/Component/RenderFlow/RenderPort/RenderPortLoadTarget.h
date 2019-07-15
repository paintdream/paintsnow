// RenderPortLoadTarget.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#ifndef __RENDERPORTLOADTARGET_H__
#define __RENDERPORTLOADTARGET_H__

#include "../RenderPort.h"

namespace PaintsNow {
	namespace NsMythForest {
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
}

#endif // __RENDERPORTLOADTARGET_H__