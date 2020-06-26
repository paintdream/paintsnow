// AudioResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __AUDIO_RESOURCE_H__
#define __AUDIO_RESOURCE_H__

#include "../ResourceBase.h"
#include "../../../General/Interface/IAudio.h"
#include "../../../General/Misc/ZLocalStream.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class AudioResource : public TReflected<AudioResource, DeviceResourceBase<IFilterBase> > {
		public:
			AudioResource(ResourceManager& manager, const String& uniqueID);
			virtual void Download(IFilterBase& device, void* deviceContext);
			virtual void Upload(IFilterBase& device, void* deviceContext);
			virtual void Attach(IFilterBase& device, void* deviceContext);
			virtual void Detach(IFilterBase& device, void* deviceContext);

			virtual bool operator << (IStreamBase& stream) override;
			virtual bool operator >> (IStreamBase& stream) const override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual IReflectObject* Clone() const override;
			IAudio::Decoder* GetAudioStream() const;

		protected:
			void Cleanup();

		protected:
			IAudio::Decoder* audioStream;
			ZLocalStream rawStream;
		};
	}
}

#endif // __AUDIO_RESOURCE_H__