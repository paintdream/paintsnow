// RenderPolicy.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-24
//

#pragma once
#include "../../../../Core/System/Tiny.h"
#include "../../../../General/Interface/IRender.h"

namespace PaintsNow {
	class RenderPolicy : public TReflected<RenderPolicy, SharedTiny> {
	public:
		RenderPolicy();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		String renderPortName;
		std::pair<uint16_t, uint16_t> priorityRange;
		IRender::Resource::RenderStateDescription renderStateTemplate;
		IRender::Resource::RenderStateDescription renderStateMask;
	};
}

