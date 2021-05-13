// RayTraceComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	class RayTraceComponent : public TAllocatedTiny<RayTraceComponent, Component> {
	public:
		RayTraceComponent();
		~RayTraceComponent() override;
	};
}

