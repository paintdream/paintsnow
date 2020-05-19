// AudioResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __AUDIO_RESOURCE_H__
#define __AUDIO_RESOURCE_H__

#include "../ResourceBase.h"
#include "../../../General/Interface/IAudio.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class AudioResource : public TReflected<AudioResource, DeviceResourceBase<IAudio> > {
		public:
			AudioResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID);
			virtual void Download(IAudio& device, void* deviceContext);
			virtual void Upload(IAudio& device, void* deviceContext);
			virtual void Attach(IAudio& device, void* deviceContext);
			virtual void Detach(IAudio& device, void* deviceContext);

			virtual bool operator << (IStreamBase& stream) override;
			virtual bool operator >> (IStreamBase& stream) const override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			IAudio::Buffer* GetAudioBuffer();

		protected:
			IAudio::Buffer* audioBuffer;
			IStreamBase* onlineStream;
			Bytes payload;
		};
	}
}

#endif // __AUDIO_RESOURCE_H__