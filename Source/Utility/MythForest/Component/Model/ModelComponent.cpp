#include "ModelComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Transform/TransformComponent.h"
#include <utility>

using namespace PaintsNow;

ModelComponent::ModelComponent(const TShared<MeshResource>& res, const TShared<BatchComponent>& batch) : batchComponent(std::move(batch)), meshResource(std::move(res)), hostCount(0) {
	Flag().fetch_or(COMPONENT_SHARED, std::memory_order_relaxed); // can be shared among different entities
}

void ModelComponent::SetMaterial(uint32_t meshGroupIndex, const TShared<MaterialResource>& materialResource) {
	assert(shaderOverriders.empty());
	materialResources.emplace_back(std::make_pair(meshGroupIndex, materialResource));
}

uint32_t ModelComponent::CreateOverrider(const TShared<ShaderResource>& shaderResourceTemplate) {
	std::vector<TShared<ShaderResource> >::iterator it = std::binary_find(shaderOverriders.begin(), shaderOverriders.end(), shaderResourceTemplate);
	if (it == shaderOverriders.end()) {
		it = std::binary_insert(shaderOverriders, shaderResourceTemplate);
	}
	
	return safe_cast<uint32_t>((it - shaderOverriders.begin() + 1) * materialResources.size());
}

static void GenerateDrawCall(IDrawCallProvider::OutputRenderData& renderData, ShaderResource* shaderResource, std::vector<IRender::Resource*>& meshBuffers, const IAsset::MeshGroup& slice, const MeshResource::BufferCollection& bufferCollection, uint32_t deviceElementSize) {
	assert(deviceElementSize != 0);
	IRender::Resource::DrawCallDescription& drawCall = renderData.drawCallDescription;
	PassBase::Updater& updater = shaderResource->GetPassUpdater();
	assert(drawCall.shaderResource == shaderResource->GetShaderResource());

	std::vector<PassBase::Parameter> outputs;
	std::vector<std::pair<uint32_t, uint32_t> > offsets;
	bufferCollection.GetDescription(outputs, offsets, updater);

	// match resource
	for (size_t k = 0; k < outputs.size(); k++) {
		if (outputs[k]) {
			uint8_t slot = outputs[k].slot;
			assert(outputs[k].offset == 0);

			if (slot >= drawCall.bufferResources.size()) drawCall.bufferResources.resize(slot + 1);
			assert(meshBuffers[k] != nullptr);
			drawCall.bufferResources[slot].buffer = meshBuffers[k];
			drawCall.bufferResources[slot].offset = offsets[k].first;
			drawCall.bufferResources[slot].component = offsets[k].second;
		}
	}

	drawCall.indexBufferResource.buffer = bufferCollection.indexBuffer;
	drawCall.indexBufferResource.length = slice.primitiveCount * deviceElementSize;
	drawCall.indexBufferResource.offset = slice.primitiveOffset * deviceElementSize;
	drawCall.indexBufferResource.type = deviceElementSize == 3 ? IRender::Resource::BufferDescription::UNSIGNED_BYTE : deviceElementSize == 6 ? IRender::Resource::BufferDescription::UNSIGNED_SHORT : IRender::Resource::BufferDescription::UNSIGNED_INT;

	IRender::Resource::RenderStateDescription& renderState = renderData.renderStateDescription;
	renderState.stencilReplacePass = 1;
	renderState.cull = 1;
	renderState.fill = 1;
	renderState.colorWrite = 1;
	renderState.blend = 0;
	renderState.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
	renderState.depthWrite = 1;
	renderState.stencilTest = IRender::Resource::RenderStateDescription::ALWAYS;
	renderState.stencilWrite = 1;
	renderState.stencilMask = 0;
	renderState.stencilValue = 0;
}

void ModelComponent::GenerateDrawCalls(std::vector<OutputRenderData>& drawCallTemplates, std::vector<std::pair<uint32_t, TShared<MaterialResource> > >& materialResources) {
	MeshResource::BufferCollection& bufferCollection = meshResource->bufferCollection;
	std::vector<IRender::Resource*> meshBuffers;
	bufferCollection.UpdateData(meshBuffers);
	InputRenderData inputRenderData;
	assert(batchComponent->GetBufferUsage() == IRender::Resource::BufferDescription::UNIFORM);

	for (size_t i = 0; i < materialResources.size(); i++) {
		std::pair<uint32_t, TShared<MaterialResource> >& mat = materialResources[i];
		uint32_t meshGroupIndex = mat.first;
		if (meshGroupIndex < meshResource->meshCollection.groups.size()) {
			IAsset::MeshGroup& slice = meshResource->meshCollection.groups[meshGroupIndex];
			TShared<MaterialResource>& materialResource = mat.second;
			if (materialResource) {
				assert(materialResource->originalShaderResource);
				drawCallTemplates.emplace_back(OutputRenderData());

				OutputRenderData& drawCall = drawCallTemplates.back();
				std::vector<Bytes> uniformBufferData;
				TShared<ShaderResource> shaderInstance = materialResource->Instantiate(meshResource, drawCall.drawCallDescription, uniformBufferData);
				drawCall.shaderResource = shaderInstance;

				uint32_t orgSize = safe_cast<uint32_t>(drawCallTemplates.size());
				std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferRanges(uniformBufferData.size());
				for (size_t n = 0; n < uniformBufferData.size(); n++) {
					const Bytes& data = uniformBufferData[n];
					if (!data.Empty()) {
						bufferRanges[n] = batchComponent->Allocate(data.GetData(), safe_cast<uint32_t>(data.GetSize()));
					}
				}

				drawCall.dataUpdater = batchComponent();

				GenerateDrawCall(drawCall, shaderInstance(), meshBuffers, slice, meshResource->bufferCollection, meshResource->deviceElementSize);
				std::vector<IRender::Resource::DrawCallDescription::BufferRange>& targetBufferRanges = drawCall.drawCallDescription.bufferResources;
				for (size_t m = 0; m < Math::Min(bufferRanges.size(), targetBufferRanges.size()); m++) {
					const IRender::Resource::DrawCallDescription::BufferRange& targetBufferRange = targetBufferRanges[m];
					if (bufferRanges[m].buffer != nullptr) {
						assert(targetBufferRanges[m].buffer == nullptr);
						targetBufferRanges[m] = bufferRanges[m];
					}
				}
			}
		}
	}
}

uint32_t ModelComponent::CollectDrawCalls(std::vector<OutputRenderData>& drawCalls, const InputRenderData& inputRenderData) {
	drawCalls.reserve(drawCalls.size() + materialResources.size());
	ShaderResource* overrideShaderTemplate = inputRenderData.overrideShaderTemplate;
	uint32_t baseIndex = 0;
	if (overrideShaderTemplate != nullptr) {
		size_t orgCount = shaderOverriders.size();
		baseIndex = CreateOverrider(overrideShaderTemplate);
		// new ones
		if (shaderOverriders.size() != orgCount) {
			drawCallTemplates.reserve(drawCallTemplates.size() + materialResources.size());
			std::vector<std::pair<uint32_t, TShared<MaterialResource> > > overrideMaterialResources;

			overrideMaterialResources.resize(materialResources.size());
			for (size_t i = 0; i < materialResources.size(); i++) {
				overrideMaterialResources[i].first = materialResources[i].first;
				overrideMaterialResources[i].second = materialResources[i].second->CloneWithOverrideShader(overrideShaderTemplate);
			}

			GenerateDrawCalls(drawCallTemplates, overrideMaterialResources);
		}
	}

	for (size_t i = 0; i < materialResources.size(); i++) {
		drawCalls.emplace_back(drawCallTemplates[i + baseIndex]);
	}

	return safe_cast<uint32_t>(materialResources.size());
}

TObject<IReflect>& ModelComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(batchComponent)[Runtime];
		ReflectProperty(meshResource)[Runtime];
		ReflectProperty(materialResources)[Runtime];
	}

	return *this;
}

void ModelComponent::Initialize(Engine& engine, Entity* entity) {
	// Allocate buffers ... 
	batchComponent->InstanceInitialize(engine);
	if (hostCount++ == 0) {
		Expand(engine);
	}

	RenderableComponent::Initialize(engine, entity);
}

void ModelComponent::Uninitialize(Engine& engine, Entity* entity) {
	RenderableComponent::Uninitialize(engine, entity);
	batchComponent->InstanceUninitialize(engine);

	// fully detached?
	if (--hostCount == 0) {
		Collapse(engine);
	}
}

String ModelComponent::GetDescription() const {
	return meshResource->GetLocation();
}

void ModelComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box) {
	const Float3Pair& sub = meshResource->GetBoundingBox();
	Union(box, sub.first);
	Union(box, sub.second);
}

void ModelComponent::Collapse(Engine& engine) {
	assert(collapseData.meshResourceLocation.empty());
	collapseData.meshResourceLocation = meshResource->GetLocation();
	collapseData.materialResourceLocations.reserve(materialResources.size());
	for (size_t i = 0; i < materialResources.size(); i++) {
		collapseData.materialResourceLocations.emplace_back(materialResources[i].second->GetLocation());
		materialResources[i].second = nullptr;
	}

	shaderOverriders.clear();
	drawCallTemplates.clear();
}

void ModelComponent::Expand(Engine& engine) {
	if (!collapseData.meshResourceLocation.empty()) {
		SnowyStream& snowyStream = engine.snowyStream;
		meshResource = snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), collapseData.meshResourceLocation);
		assert(collapseData.materialResourceLocations.size() == materialResources.size());
		for (size_t i = 0; i < materialResources.size(); i++) {
			assert(materialResources[i].second);
			materialResources[i].second = snowyStream.CreateReflectedResource(UniqueType<MaterialResource>(), collapseData.materialResourceLocations[i]);
		}

		collapseData.meshResourceLocation = "";
		collapseData.materialResourceLocations.clear();
	}

	// inspect vertex format
	assert(drawCallTemplates.empty());
	drawCallTemplates.reserve(materialResources.size());
	GenerateDrawCalls(drawCallTemplates, materialResources);
}

size_t ModelComponent::ReportGraphicMemoryUsage() const {
	size_t size = meshResource->ReportDeviceMemoryUsage();
	for (size_t i = 0; i < materialResources.size(); i++) {
		size += materialResources[i].second->ReportDeviceMemoryUsage();
	}

	return size;
}
