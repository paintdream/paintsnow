#include "MaterialResource.h"
#include "../ResourceManager.h"
#include <sstream>

using namespace PaintsNow;

MaterialResource::MaterialResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

TShared<ShaderResource> MaterialResource::Instantiate(const TShared<MeshResource>& mesh, IRender::Resource::DrawCallDescription& drawCallTemplate, std::vector<Bytes>& bufferData) {
	assert(mesh);

	TShared<ShaderResource> shaderTemplateResource = originalShaderResource;
	Bytes templateHash = shaderTemplateResource->GetHashValue();

	while (true) {
		TShared<ShaderResource> mutationShaderResource;
		mutationShaderResource.Reset(static_cast<ShaderResource*>(shaderTemplateResource->Clone()));
		assert(mutationShaderResource->GetPassUpdater().GetBufferCount() != 0);
		PassBase& pass = mutationShaderResource->GetPass();
		PassBase::Updater& updater = mutationShaderResource->GetPassUpdater();
		mutationShaderResource->GetPass().ClearBindings();

		// Apply material
		for (size_t i = 0; i < materialParams.variables.size(); i++) {
			const IAsset::Material::Variable& var = materialParams.variables[i];
			PassBase::Parameter& parameter = const_cast<PassBase::Parameter&>(updater[var.key]);
			if (parameter) {
				if (var.type == IAsset::TYPE_TEXTURE) {
					// Lookup texture 
					IAsset::TextureIndex textureIndex = var.Parse(UniqueType<IAsset::TextureIndex>());
					IRender::Resource* texture = nullptr;
					assert(textureIndex.index < textureResources.size());
					if (textureIndex.index < textureResources.size()) {
						TShared<TextureResource>& res = textureResources[textureIndex.index];
						assert(res);
						if (res) {
							texture = res->GetRenderResource();
						}
					}

					parameter = texture;
				} else {
					parameter = var.value;
				}
			}
		}

		// apply buffers
		std::vector<PassBase::Parameter> descs;
		std::vector<std::pair<uint32_t, uint32_t> > offsets;
		mesh->bufferCollection.GetDescription(descs, offsets, updater);
		std::vector<IRender::Resource*> data;
		mesh->bufferCollection.UpdateData(data);
		assert(data.size() == descs.size());

		for (size_t k = 0; k < descs.size(); k++) {
			IShader::BindBuffer* bindBuffer = descs[k].bindBuffer;
			if (bindBuffer != nullptr) {
				bindBuffer->resource = data[k];
			}
		}

		// get shader hash
		pass.FlushOptions();
		Bytes shaderHash = pass.ExportHash();

		if (templateHash == shaderHash) { // matched!
			updater.Capture(drawCallTemplate, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
			for (size_t k = 0; k < descs.size(); k++) {
				assert(descs[k].slot < drawCallTemplate.bufferResources.size());
			}

			drawCallTemplate.shaderResource = shaderTemplateResource->GetShaderResource();
			assert(shaderTemplateResource->GetPassUpdater().GetBufferCount() != 0);
			return shaderTemplateResource;
		} else {
			templateHash = shaderHash;
			// create new shader mutation
			String location = originalShaderResource->GetLocation();
			location += "/";
			for (size_t i = 0; i < shaderHash.GetSize(); i++) {
				shaderHash[i] += (uint8_t)'0';
			}

			location.append((const char*)shaderHash.GetData(), shaderHash.GetSize());

			TShared<ShaderResource> cached = static_cast<ShaderResource*>(resourceManager.LoadExistSafe(location)());

			if (cached) {
				// use cache
				mutationShaderResource = cached;
			} else {
				mutationShaderResource.Reset(static_cast<ShaderResource*>(mutationShaderResource->Clone()));
			}

			// cached?
			resourceManager.DoLock();
			cached = static_cast<ShaderResource*>(resourceManager.LoadExist(location)());

			if (cached) {
				// use cache
				mutationShaderResource = cached;
			} else {
				mutationShaderResource->SetLocation(location);
				resourceManager.Insert(mutationShaderResource());
			}

			resourceManager.UnLock();
			shaderTemplateResource = mutationShaderResource;
		}
	}
}

void MaterialResource::Attach(IRender& render, void* deviceContext) {}

void MaterialResource::Detach(IRender& render, void* deviceContext) {}

void MaterialResource::Upload(IRender& render, void* deviceContext) {
	Flag().fetch_or(RESOURCE_UPLOADED, std::memory_order_release);
}

void MaterialResource::Download(IRender& render, void* deviceContext) {}

size_t MaterialResource::ReportDeviceMemoryUsage() const {
	size_t size = 0;
	for (size_t i = 0; i < textureResources.size(); i++) {
		const TShared<TextureResource>& texture = textureResources[i];
		size += texture->ReportDeviceMemoryUsage();
	}

	return size;
}

TShared<MaterialResource> MaterialResource::CloneWithOverrideShader(const TShared<ShaderResource>& overrideShaderResource) {
	if (overrideShaderResource == originalShaderResource) {
		return this;
	} else {
		assert(overrideShaderResource);
		// create overrider
		TShared<MaterialResource> clone;
		if (uniqueLocation.size() != 0) {
			String overrideLocation = uniqueLocation + "@(" + overrideShaderResource->GetLocation() + ")";
			resourceManager.DoLock();
			clone = static_cast<MaterialResource*>(resourceManager.LoadExist(overrideLocation)());

			if (!clone) {
				clone = TShared<MaterialResource>::From(new MaterialResource(resourceManager, overrideLocation));
				clone->materialParams = materialParams;
				clone->textureResources = textureResources;
				clone->originalShaderResource = overrideShaderResource;
				clone->SetLocation(overrideLocation);

				resourceManager.Insert(clone());
			}

			resourceManager.UnLock();
		} else {
			// Orphan
			clone = TShared<MaterialResource>::From(new MaterialResource(resourceManager, ""));
			clone->materialParams = materialParams;
			clone->textureResources = textureResources;
			clone->originalShaderResource = overrideShaderResource;
		}

		return clone;
	}
}


TObject<IReflect>& MaterialResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(materialParams);
		ReflectProperty(originalShaderResource)[MetaResourceInternalPersist(resourceManager)];
		ReflectProperty(textureResources)[MetaResourceInternalPersist(resourceManager)];
	}

	return *this;
}
