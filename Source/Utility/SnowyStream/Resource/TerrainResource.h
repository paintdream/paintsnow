// TerrainResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "TextureResource.h"

namespace PaintsNow {
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

