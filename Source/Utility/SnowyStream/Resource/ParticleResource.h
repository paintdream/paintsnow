// ParticleResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __PARTICLE_RESOURCE_H__
#define __PARTICLE_RESOURCE_H__

#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ParticleResource : public TReflected<ParticleResource, GraphicResourceBase> {
		public:
			ParticleResource(ResourceManager& manager, const String& uniqueID);
			virtual bool operator << (IStreamBase& stream) override;
			virtual bool operator >> (IStreamBase& stream) const override;
			virtual void Upload(IRender& render, void* deviceContext) override;
			virtual void Download(IRender& render, void* deviceContext) override;
			virtual void Attach(IRender& render, void* deviceContext) override;
			virtual void Detach(IRender& render, void* deviceContext) override;

		};
	}
}

#endif // __PARTICLE_RESOURCE_H__