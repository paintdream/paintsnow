// TextResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-11
//

#ifndef __TEXTRESOURCE_H__
#define __TEXTRESOURCE_H__

#include "../ResourceBase.h"
#include "../../../Core/Interface/IArchive.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class TextResource : public TReflected<TextResource, DeviceResourceBase<IArchive> > {
		public:
			TextResource(ResourceManager& manager, const String& uniqueID);
			virtual ~TextResource();
			virtual bool LoadExternalResource(IStreamBase& streamBase, size_t length);

			// re-pure-virtualize these interfaces
			virtual void Upload(IArchive& device, void* deviceContext);
			virtual void Download(IArchive& device, void* deviceContext);
			virtual void Attach(IArchive& device, void* deviceContext);
			virtual void Detach(IArchive& device, void* deviceContext);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		protected:
			String text;
		};
	}
}


#endif // __TEXTRESOURCE_H__