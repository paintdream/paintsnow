// AnalyticCurveResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __ANALYTICCURVE_RESOURCE_H__
#define __ANALYTICCURVE_RESOURCE_H__

#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class AnalyticCurveResource : public TReflected<AnalyticCurveResource, GraphicResourceBase> {
		public:
			AnalyticCurveResource(ResourceManager& manager, const String& uniqueID);
		};
	}
}

#endif // __ANALYTICCURVE_RESOURCE_H__