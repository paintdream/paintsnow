// MaterialResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "GraphicResourceBase.h"
#include "ShaderResource.h"
#include "TextureResource.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class MaterialResource;
	class IDrawCallProvider {
	public:
		struct InputRenderData {
			InputRenderData(float ref = 0.0f, ShaderResource* res = nullptr) : overrideShaderTemplate(res), viewReference(ref) {}
			ShaderResource* overrideShaderTemplate;
			float viewReference;
		};

		struct OutputRenderData {
			IRender::Resource::DrawCallDescription drawCallDescription;
			IRender::Resource::RenderStateDescription renderStateDescription;
			IDataUpdater* dataUpdater;
			TShared<ShaderResource> shaderResource;
			TShared<SharedTiny> host;
			std::vector<MatrixFloat4x4> localTransforms;
			std::vector<std::pair<uint32_t, Bytes> > localInstancedData;
		};

		virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) = 0;
	};

	class MaterialResource : public TReflected<MaterialResource, GraphicResourceBase>, public IDrawCallProvider {
	public:
		MaterialResource(ResourceManager& manager, const String& uniqueID);

		size_t ReportDeviceMemoryUsage() const override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;

		TShared<MaterialResource> CloneWithOverrideShader(TShared<ShaderResource> override);

		IAsset::Material materialParams;
		TShared<ShaderResource> originalShaderResource;
		TShared<ShaderResource> mutationShaderResource;
		std::vector<TShared<TextureResource> > textureResources;
		IRender::Resource::DrawCallDescription drawCallTemplate;
		std::vector<Bytes> bufferData;
	};
}

