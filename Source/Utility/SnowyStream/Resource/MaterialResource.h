// MaterialResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "GraphicResourceBase.h"
#include "ShaderResource.h"
#include "TextureResource.h"
#include "MeshResource.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class MaterialResource : public TReflected<MaterialResource, GraphicResourceBase> {
	public:
		MaterialResource(ResourceManager& manager, const String& uniqueID);

		size_t ReportDeviceMemoryUsage() const override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ShaderResource> Instantiate(const TShared<MeshResource>& mesh, IRender::Resource::DrawCallDescription& drawCallTemplate);
		TShared<MaterialResource> CloneWithOverrideShader(const TShared<ShaderResource>& override);

		IAsset::Material materialParams;
		TShared<ShaderResource> originalShaderResource;
		std::vector<TShared<TextureResource> > textureResources;
		std::vector<Bytes> bufferData;
	};
}

