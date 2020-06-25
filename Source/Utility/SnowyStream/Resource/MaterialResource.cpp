#include "MaterialResource.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

MaterialResource::MaterialResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID) {}

void MaterialResource::Attach(IRender& render, void* deviceContext) {
	// Collect empty draw calls
	if (originalShaderResource) {
		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
		// duplicate shader resource, do not share them
		mutationShaderResource.Reset(static_cast<ShaderResource*>(originalShaderResource->Clone()));
		ZPassBase& pass = mutationShaderResource->GetPass();
		ZPassBase::Updater& updater = mutationShaderResource->GetPassUpdater();
		// Apply material
		for (size_t i = 0; i < materialParams.variables.size(); i++) {
			const IAsset::Material::Variable& var = materialParams.variables[i];
			if (var.type == IAsset::TYPE_TEXTURE) {
				// Lookup texture 
				IAsset::TextureIndex textureIndex = var.Parse(UniqueType<IAsset::TextureIndex>());
				IRender::Resource* texture = nullptr;
				assert(textureIndex.index < textureResources.size());
				if (textureIndex.index < textureResources.size()) {
					TShared<TextureResource>& res = textureResources[textureIndex.index];
					assert(res);
					if (res) {
						texture = res->GetTexture();
					}
				}

				updater[var.key] = texture;
			} else {
				updater[var.key] = var.value;
			}
		}

		updater.Capture(drawCallTemplate, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);

		// get shader hash
		Bytes shaderHash = pass.ExportHash(true);
		if (originalShaderResource->GetHashValue() == shaderHash) {
			// use original
			mutationShaderResource = originalShaderResource;
		} else {
			String location = originalShaderResource->GetLocation();
			location += "/";
			location.append((const char*)shaderHash.GetData(), shaderHash.GetSize());

			// cached?
			resourceManager.DoLock();
			TShared<ShaderResource> cached = static_cast<ShaderResource*>(resourceManager.LoadExist(location)());

			if (cached) {
				// use cache
				mutationShaderResource = cached;
			} else {
				mutationShaderResource->SetLocation(location);
				resourceManager.Insert(mutationShaderResource());
			}

			resourceManager.UnLock();
		}

		drawCallTemplate.shaderResource = originalShaderResource->GetShaderResource();
	}
}

void MaterialResource::Detach(IRender& render, void* deviceContext) {
}

void MaterialResource::Upload(IRender& render, void* deviceContext) {

}

void MaterialResource::Download(IRender& render, void* deviceContext) {

}

size_t MaterialResource::ReportDeviceMemoryUsage() const {
	size_t size = 0;
	for (size_t i = 0; i < textureResources.size(); i++) {
		const TShared<TextureResource>& texture = textureResources[i];
		size += texture->ReportDeviceMemoryUsage();
	}

	// bufferData will be uploaded further in constructing drawcalls.
	for (size_t j = 0; j < bufferData.size(); j++) {
		size += bufferData[j].GetSize();
	}

	return size;
}

TShared<MaterialResource> MaterialResource::CloneWithOverrideShader(TShared<ShaderResource> overrideShaderResource) {
	if (overrideShaderResource == originalShaderResource) {
		return this;
	} else {
		// create overrider
		ResourceManager::UniqueLocation overrideLocation = uniqueLocation + "@(" + overrideShaderResource->GetLocation() + ")";
		resourceManager.DoLock();
		TShared<MaterialResource> clone = static_cast<MaterialResource*>(resourceManager.LoadExist(overrideLocation)());

		if (!clone) {
			clone = TShared<MaterialResource>::From(new MaterialResource(resourceManager, overrideLocation));
			clone->materialParams = materialParams;
			clone->textureResources = textureResources;
			clone->originalShaderResource = overrideShaderResource;
			clone->SetLocation(overrideLocation);

			resourceManager.Insert(clone());
		}

		resourceManager.UnLock();

		return clone;
	}
}

uint32_t MaterialResource::CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) {
	if (mutationShaderResource) {
		// uint32_t count = mutationShaderResource->CollectDrawCalls(outputDrawCalls, inputRenderData);
		OutputRenderData renderData;
		renderData.shaderResource = mutationShaderResource();
		renderData.drawCallDescription = drawCallTemplate;
		renderData.host = this;
		outputDrawCalls.emplace_back(std::move(renderData));

		return 1;
	} else {
		return 0;
	}
}

TObject<IReflect>& MaterialResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(materialParams);
		ReflectProperty(originalShaderResource)[MetaResourceInternalPersist(resourceManager)];
		ReflectProperty(mutationShaderResource)[Runtime];
		ReflectProperty(textureResources)[MetaResourceInternalPersist(resourceManager)];
		ReflectProperty(drawCallTemplate)[Runtime];
	}

	return *this;
}
