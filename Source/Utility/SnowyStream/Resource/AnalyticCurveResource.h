// AnalyticCurveResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class AnalyticCurveResource : public TReflected<AnalyticCurveResource, GraphicResourceBase> {
	public:
		AnalyticCurveResource(ResourceManager& manager, const String& uniqueID);
	};
}

