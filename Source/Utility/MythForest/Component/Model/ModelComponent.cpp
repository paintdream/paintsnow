#include "ModelComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Transform/TransformComponent.h"
#include <utility>

using namespace PaintsNow;

ModelComponent::ModelComponent(const TShared<MeshResource>& res, const TShared<BatchComponent>& batch) : batchComponent(std::move(batch)), meshResource(std::move(res)), hostCount(0) {
	assert(res);
	Flag().fetch_or(COMPONENT_SHARED, std::memory_order_relaxed); // can be shared among different entities
}

void ModelComponent::SetMaterial(uint16_t meshGroupIndex, uint16_t priority, const TShared<MaterialResource>& materialResource) {
	assert(shaderOverriders.empty());
	materialResources.emplace_back(std::make_pair(((uint32_t)priority << 16) | meshGroupIndex, materialResource));
}

uint32_t ModelComponent::CreateOverrider(const TShared<ShaderResource>& shaderResourceTemplate) {
	std::vector<TShared<ShaderResource> >::iterator it = std::binary_find(shaderOverriders.begin(), shaderOverriders.end(), shaderResourceTemplate);
	if (it == shaderOverriders.end()) {
		it = std::binary_insert(shaderOverriders, shaderResourceTemplate);
	}
	
	return verify_cast<uint32_t>((it - shaderOverriders.begin() + 1) * materialResources.size());
}

const std::vector<std::pair<uint32_t, TShared<MaterialResource> > >& ModelComponent::GetMaterials() const {
	return materialResources;
}

static void GenerateDrawCall(IDrawCallProvider::OutputRenderData& renderData, ShaderResource* shaderResource, std::vector<IRender::Resource*>& meshBuffers, const IAsset::MeshGroup& slice, const MeshResource::BufferCollection& bufferCollection, uint32_t deviceElementSize, uint16_t priority, uint16_t index) {
	assert(deviceElementSize != 0);
	IRender::Resource::DrawCallDescription& drawCall = renderData.drawCallDescription;
	PassBase::Updater& updater = shaderResource->GetPassUpdater();
	assert(updater.GetBufferCount() != 0);
	assert(drawCall.shaderResource == shaderResource->GetShaderResource());
	renderData.priority = priority;
	renderData.groupIndex = index;

	std::vector<PassBase::Parameter> outputs;
	std::vector<std::pair<uint32_t, uint32_t> > offsets;
	bufferCollection.GetDescription(outputs, offsets, updater);

	// match resource
	for (size_t k = 0; k < outputs.size(); k++) {
		if (outputs[k]) {
			uint8_t slot = outputs[k].slot;
			assert(outputs[k].offset == 0);
			assert(slot < drawCall.bufferResources.size());
			assert(meshBuffers[k] != nullptr);

			IRender::Resource::DrawCallDescription::BufferRange& bufferRange = drawCall.bufferResources[slot];
			bufferRange.buffer = meshBuffers[k];
			bufferRange.offset = offsets[k].first;
			bufferRange.component = offsets[k].second;
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
		uint16_t meshGroupIndex = verify_cast<uint16_t>(mat.first & 0xFFFF);
		uint16_t priority = verify_cast<uint16_t>(mat.first >> 16);

		if (meshGroupIndex < meshResource->meshCollection.groups.size()) {
			IAsset::MeshGroup& slice = meshResource->meshCollection.groups[meshGroupIndex];
			TShared<MaterialResource>& materialResource = mat.second;
			if (materialResource) {
				assert(materialResource->originalShaderResource);
				drawCallTemplates.emplace_back(OutputRenderData());

				OutputRenderData& drawCall = drawCallTemplates.back();
				std::vector<Bytes> uniformBufferData;
				TShared<ShaderResource> shaderInstance = materialResource->Instantiate(meshResource, drawCall.drawCallDescription, uniformBufferData);
				assert(shaderInstance->GetPassUpdater().GetBufferCount() != 0);
				drawCall.shaderResource = shaderInstance;

				uint32_t orgSize = verify_cast<uint32_t>(drawCallTemplates.size());
				std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferRanges(uniformBufferData.size());
				for (size_t n = 0; n < uniformBufferData.size(); n++) {
					const Bytes& data = uniformBufferData[n];
					if (!data.Empty()) {
						bufferRanges[n] = batchComponent->Allocate(data.GetData(), verify_cast<uint32_t>(data.GetSize()));
					}
				}

				drawCall.dataUpdater = batchComponent();

				GenerateDrawCall(drawCall, shaderInstance(), meshBuffers, slice, meshResource->bufferCollection, meshResource->deviceElementSize, priority, meshGroupIndex);
				std::vector<IRender::Resource::DrawCallDescription::BufferRange>& targetBufferRanges = drawCall.drawCallDescription.bufferResources;
				for (size_t m = 0; m < Math::Min(bufferRanges.size(), (size_t)targetBufferRanges.size()); m++) {
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

uint32_t ModelComponent::CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& drawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) {
	if (drawCallTemplates.empty())
		return 0;

	if (!(meshResource->Flag().load(std::memory_order_relaxed) & ResourceBase::RESOURCE_UPLOADED))
		return ~(uint32_t)0;

	assert(!(Flag().fetch_or(Tiny::TINY_PINNED) & Tiny::TINY_PINNED));
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

	if (!(option & CollectOption::COLLECT_AGILE_RENDERING)) {
		for (size_t i = 0; i < materialResources.size(); i++) {
			IDrawCallProvider::OutputRenderData& drawCall = drawCallTemplates[i + baseIndex];
			if (!(drawCall.shaderResource->Flag().load(std::memory_order_relaxed) & ResourceBase::RESOURCE_UPLOADED)) {
				assert(Flag().fetch_and(~Tiny::TINY_PINNED) & Tiny::TINY_PINNED);
				return ~(uint32_t)0;
			}
		}
	}

	drawCalls.reserve(drawCalls.size() + materialResources.size());
	for (size_t i = 0; i < materialResources.size(); i++) {
		IDrawCallProvider::OutputRenderData& drawCall = drawCallTemplates[i + baseIndex];
		if (drawCall.shaderResource->Flag().load(std::memory_order_relaxed) & ResourceBase::RESOURCE_UPLOADED) {
			drawCalls.emplace_back(drawCallTemplates[i + baseIndex]); // TODO: optimize copy performance
		}
	}

	assert(Flag().fetch_and(~Tiny::TINY_PINNED) & Tiny::TINY_PINNED);
	return verify_cast<uint32_t>(materialResources.size());
}

TObject<IReflect>& ModelComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(meshResource)[Runtime];
		ReflectProperty(batchComponent)[Runtime];
		ReflectProperty(shaderOverriders)[Runtime];
		ReflectProperty(materialResources)[Runtime];
		ReflectProperty(drawCallTemplates)[Runtime];
		ReflectProperty(collapseData)[Runtime];
		ReflectProperty(hostCount)[Runtime];
	}

	return *this;
}

void ModelComponent::Initialize(Engine& engine, Entity* entity) {
	// Allocate buffers ... 
	batchComponent->InstanceInitialize(engine);
	if (hostCount++ == 0) {
		Expand(engine);
	}

	BaseClass::Initialize(engine, entity);
}

void ModelComponent::Uninitialize(Engine& engine, Entity* entity) {
	BaseClass::Uninitialize(engine, entity);
	batchComponent->InstanceUninitialize(engine);

	// fully detached?
	if (--hostCount == 0) {
		Collapse(engine);
	}
}

String ModelComponent::GetDescription() const {
	return meshResource->GetLocation();
}

void ModelComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) {
	const Float3Pair& sub = meshResource->GetBoundingBox();

	assert(sub.first.x() > -FLT_MAX && sub.second.x() < FLT_MAX);
	assert(sub.first.y() > -FLT_MAX && sub.second.y() < FLT_MAX);
	assert(sub.first.z() > -FLT_MAX && sub.second.z() < FLT_MAX);

	if (sub.first.x() <= sub.second.x()) {
		Math::Union(box, sub.first);
		Math::Union(box, sub.second);
	}
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
