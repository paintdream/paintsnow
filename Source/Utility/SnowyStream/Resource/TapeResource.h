// TapeResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __TAPE_RESOURCE_H__
#define __TAPE_RESOURCE_H__

#include "../ResourceBase.h"
#include "../../../Core/Interface/IArchive.h"
#include "../../../General/Misc/ZPacket.h"
#include "../../../General/Misc/ZLocalStream.h"

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
			virtual bool operator << (IStreamBase& stream) override;
			virtual bool operator >> (IStreamBase& stream) const override;
			virtual IReflectObject* Clone() const override;

		protected:
			void Close();
			ZPacket* packet;
			ZLocalStream localStream;
		};
	}
}

#endif // __TAPE_RESOURCE_H__