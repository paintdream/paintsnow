#include "ModelComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Transform/TransformComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ModelComponent::ModelComponent(TShared<MeshResource>& res, TShared<BatchComponent>& batch) : batchComponent(batch), meshResource(res) {
	Flag() |= COMPONENT_SHARED; // can be shared among different entities
}

void ModelComponent::AddMaterial(uint32_t meshGroupIndex, TShared<MaterialResource>& materialResource) {
	materialResources.emplace_back(std::make_pair(meshGroupIndex, materialResource));
}

static void GenerateDrawCall(IDrawCallProvider::OutputRenderData& renderData, ShaderResource* shaderResource, std::vector<IRender::Resource*>& meshBuffers, const IAsset::MeshGroup& slice, const MeshResource::BufferCollection& bufferCollection) {
	IRender::Resource::DrawCallDescription& drawCall = renderData.drawCallDescription;
	ZPassBase::Updater& updater = shaderResource->GetPassUpdater();
	drawCall.shaderResource = shaderResource->GetShaderResource();

	std::vector<ZPassBase::Parameter> outputs;
	bufferCollection.GetDescription(outputs, updater);

	// match resource
	for (size_t k = 0; k < outputs.size(); k++) {
		if (outputs[k]) {
			uint8_t slot = outputs[k].slot;
			if (slot >= drawCall.bufferResources.size()) drawCall.bufferResources.resize(slot + 1);
			drawCall.bufferResources[slot].buffer = meshBuffers[k];
		}
	}

	drawCall.indexBufferResource.buffer = bufferCollection.indexBuffer;
	drawCall.indexBufferResource.length = slice.primitiveCount * sizeof(Int3);
	drawCall.indexBufferResource.offset = slice.primitiveOffset * sizeof(Int3);

	IRender::Resource::RenderStateDescription& renderState = renderData.renderStateDescription;
	renderState.pass = 0;
	renderState.cull = 1;
	renderState.fill = 1;
	renderState.colorWrite = 1;
	renderState.alphaBlend = 0;
	renderState.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
	renderState.depthWrite = 1;
	renderState.stencilTest = IRender::Resource::RenderStateDescription::ALWAYS;
	renderState.stencilWrite = 1;
	renderState.stencilMask = 0;
	renderState.stencilValue = 0;
}

uint32_t ModelComponent::CollectDrawCalls(std::vector<OutputRenderData>& drawCalls, const InputRenderData& inputRenderData) {
	if (batchComponent->Flag() & Tiny::TINY_MODIFIED) {
		return 0;
	} else {
		drawCalls.reserve(drawCalls.size() + drawCallTemplates.size());
		ShaderResource* overrideMaterialTemplate = inputRenderData.overrideMaterialTemplate;
		if (overrideMaterialTemplate == nullptr) {
			for (size_t i = 0; i < drawCallTemplates.size(); i++) {
				drawCalls.emplace_back(drawCallTemplates[i]);
			}

			return safe_cast<uint32_t>(drawCallTemplates.size());
		} else {
			MeshResource::BufferCollection& bufferCollection = meshResource->bufferCollection;

			std::vector<IRender::Resource*> meshBuffers;
			bufferCollection.UpdateData(meshBuffers);
			uint32_t baseCount = safe_cast<uint32_t>(drawCalls.size());
			drawCalls.resize(baseCount + meshResource->meshCollection.groups.size());

			for (size_t i = 0; i < meshResource->meshCollection.groups.size(); i++) {
				IAsset::MeshGroup& slice = meshResource->meshCollection.groups[i];
				OutputRenderData& renderData = drawCalls[baseCount + i];
				GenerateDrawCall(renderData, overrideMaterialTemplate, meshBuffers, slice, meshResource->bufferCollection);
			}

			return safe_cast<uint32_t>(meshResource->meshCollection.groups.size());
		}
	}
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

	if (!(Flag() & Tiny::TINY_PINNED)) {
		Flag() |= Tiny::TINY_PINNED;

		MatrixFloat4x4 identity;
		InputRenderData inputRenderData;
		uint32_t materialCount = safe_cast<uint32_t>(materialResources.size());

		// inspect vertex format
		std::vector<ZPassBase::Name> inputs;
		MeshResource::BufferCollection& bufferCollection = meshResource->bufferCollection;
		std::vector<IRender::Resource*> meshBuffers;
		bufferCollection.UpdateData(meshBuffers);
		assert(drawCallTemplates.empty());

		drawCallTemplates.reserve(materialResources.size());
		for (size_t i = 0; i < materialResources.size(); i++) {
			std::pair<uint32_t, TShared<MaterialResource> >& mat = materialResources[i];
			uint32_t meshGroupIndex = mat.first;
			if (meshGroupIndex < meshResource->meshCollection.groups.size()) {
				IAsset::MeshGroup& slice = meshResource->meshCollection.groups[meshGroupIndex];
				TShared<MaterialResource>& materialResource = mat.second;

				if (materialResource && materialResource->mutationShaderResource) {
					uint32_t orgSize = safe_cast<uint32_t>(drawCallTemplates.size());
					uint32_t subDrawCalls = materialResource->CollectDrawCalls(drawCallTemplates, inputRenderData);
					const std::vector<Bytes>& uniformBufferData = materialResource->GetBufferData();
					std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferRanges(uniformBufferData.size());
					IRender& render = engine.interfaces.render;
					for (size_t n = 0; n < uniformBufferData.size(); n++) {
						const Bytes& data = uniformBufferData[n];
						if (!data.Empty()) {
							bufferRanges[n] = batchComponent->Allocate(render, uniformBufferData[n]);
						}
					}

					for (uint32_t k = 0; k < subDrawCalls; k++) {
						OutputRenderData& renderData = drawCallTemplates[k + orgSize];
						GenerateDrawCall(renderData, materialResource->mutationShaderResource(), meshBuffers, slice, meshResource->bufferCollection);
						std::vector<IRender::Resource::DrawCallDescription::BufferRange>& targetBufferRanges = renderData.drawCallDescription.bufferResources;
						for (size_t m = 0; m < Min(bufferRanges.size(), targetBufferRanges.size()); m++) {
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
	}

	RenderableComponent::Initialize(engine, entity);
}

void ModelComponent::Uninitialize(Engine& engine, Entity* entity) {
	RenderableComponent::Uninitialize(engine, entity);
	batchComponent->InstanceUninitialize(engine);
}


String ModelComponent::GetDescription() const {
	return meshResource->GetLocation();
}

void ModelComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box) {
	const Float3Pair& sub = meshResource->GetBoundingBox();
	Union(box, sub.first);
	Union(box, sub.second);
}
