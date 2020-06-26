// TerrainResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __TERRAINRESOURCE_H__
#define __TERRAINRESOURCE_H__

#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "TextureResource.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class TerrainResource : public TReflected<TerrainResource, GraphicResourceBase> {
		public:
			TerrainResource(ResourceManager& manager, const String& uniqueID);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void Upload(IRender& render, void* deviceContext) override;
			virtual void Download(IRender& render, void* deviceContext) override;
			virtual void Attach(IRender& render, void* deviceContext) override;
			virtual void Detach(IRender& render, void* deviceContext) override;
			void FromTexture(TShared<TextureResource> textureResource);

		protected:
			std::vector<float> terrainData;
			uint32_t width, height;
			float deltaHeight;
		};
	}
}

#endif // __TERRAINRESOURCE_H__