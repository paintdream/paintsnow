// TapeResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __TAPE_RESOURCE_H__
#define __TAPE_RESOURCE_H__

#include "../ResourceBase.h"
#include "../../../Core/Interface/IArchive.h"
#include "../../../General/Misc/ZPacket.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class TapeResource : public TReflected<TapeResource, DeviceResourceBase<IArchive> > {
		public:
			TapeResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID);
			virtual ~TapeResource();

			virtual void Download(IArchive& device, void* deviceContext);
			virtual void Upload(IArchive& device, void* deviceContext);
			virtual void Attach(IArchive& device, void* deviceContext);
			virtual void Detach(IArchive& device, void* deviceContext);

			ZPacket* GetPacket();

		protected:
			void Close();
			ZPacket* packetProvider;
			IStreamBase* packetStream;
			String streamPath;
		};
	}
}

#endif // __TAPE_RESOURCE_H__