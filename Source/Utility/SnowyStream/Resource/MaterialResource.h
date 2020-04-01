// MaterialResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __MATERIAL_RESOURCE_H__
#define __MATERIAL_RESOURCE_H__

#include "GraphicResourceBase.h"
#include "ShaderResource.h"
#include "TextureResource.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MaterialResource;
		class IDrawCallProvider {
		public:
			struct InputRenderData {
				InputRenderData(float ref = 0.0f, ShaderResource* res = nullptr) :  overrideShaderTemplate(res), viewReference(ref) {}
				ShaderResource* overrideShaderTemplate;
				float viewReference;
			};

			class DataUpdater {
			public:
				virtual void Update(IRender& render, IRender::Queue* queue) = 0;
			};

			struct OutputRenderData {
				IRender::Resource::DrawCallDescription drawCallDescription;
				IRender::Resource::RenderStateDescription renderStateDescription;
				DataUpdater* dataUpdater;
				TShared<ShaderResource> shaderResource;
				TShared<SharedTiny> host;
			};

			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) = 0;
		};

		class MaterialResource : public TReflected<MaterialResource, GraphicResourceBase>, public IDrawCallProvider {
		public:
			MaterialResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID);

			virtual uint64_t GetMemoryUsage() const;
			virtual void Upload(IRender& render, void* deviceContext) override;
			virtual void Download(IRender& render, void* deviceContext) override;
			virtual void Attach(IRender& render, void* deviceContext) override;
			virtual void Detach(IRender& render, void* deviceContext) override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;

			TShared<MaterialResource> CloneWithOverrideShader(TShared<ShaderResource> override);

			IAsset::Material materialParams;
			TShared<ShaderResource> originalShaderResource;
			TShared<ShaderResource> mutationShaderResource;
			std::vector<TShared<TextureResource> > textureResources;
			IRender::Resource::DrawCallDescription drawCallTemplate;
			std::vector<Bytes> bufferData;
		};
	}
}

#endif // __MATERIAL_RESOURCE_H__