// VolumeResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __VOLUME_RESOURCE_H__
#define __VOLUME_RESOURCE_H__

#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class VolumeResource : public TReflected<VolumeResource, GraphicResourceBase> {
		public:
			VolumeResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID);
			virtual bool operator << (IStreamBase& stream) override;
			virtual bool operator >> (IStreamBase& stream) const override;
			virtual void Upload(IRender& render, void* deviceContext) override;
			virtual void Download(IRender& render, void* deviceContext) override;
			virtual void Attach(IRender& render, void* deviceContext) override;
			virtual void Detach(IRender& render, void* deviceContext) override;
		};
	}
}

#endif // __VOLUME_RESOURCE_H__