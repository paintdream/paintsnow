// StreamResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __STREAM_RESOURCE_H__
#define __STREAM_RESOURCE_H__

#include "../ResourceBase.h"
#include "../../../Core/Interface/IArchive.h"
#include "../../../Core/System/ShadowStream.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class StreamResource : public TReflected<StreamResource, DeviceResourceBase<IArchive> > {
		public:
			StreamResource(ResourceManager& manager, const String& uniqueID);

			virtual void Download(IArchive& device, void* deviceContext);
			virtual void Upload(IArchive& device, void* deviceContext);
			virtual void Attach(IArchive& device, void* deviceContext);
			virtual void Detach(IArchive& device, void* deviceContext);

			IStreamBase& GetStream();
			virtual bool operator << (IStreamBase& stream) override;
			virtual bool operator >> (IStreamBase& stream) const override;
			virtual IReflectObject* Clone() const override;

		protected:
			ShadowStream shadowStream;
		};
	}
}

#endif // __STREAM_RESOURCE_H__