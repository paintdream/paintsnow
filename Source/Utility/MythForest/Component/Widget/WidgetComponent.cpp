#include "WidgetComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

using namespace PaintsNow;

WidgetComponent::WidgetComponent(const TShared<MeshResource>& mesh, const TShared<BatchComponent>& batch, const TShared<BatchComponent>& batchInstanced) : BaseClass(mesh, batch) {
	batchInstancedDataComponent = batchInstanced;
	Flag().fetch_or(RENDERABLECOMPONENT_CAMERAVIEW, std::memory_order_relaxed);
}

void WidgetComponent::GenerateDrawCalls(std::vector<OutputRenderData>& drawCallTemplates, std::vector<std::pair<uint32_t, TShared<MaterialResource> > >& materialResources) {
	size_t origCount = drawCallTemplates.size();
	GenerateDrawCalls(drawCallTemplates, materialResources);

	// Add customized parameters!
	for (size_t i = origCount; i < drawCallTemplates.size(); i++) {
		IDrawCallProvider::OutputRenderData& renderData = drawCallTemplates[i];
		IRender::Resource::DrawCallDescription& drawCall = renderData.drawCallDescription;
		PassBase::Updater& updater = renderData.shaderResource->GetPassUpdater();

		// texture
		const PassBase::Parameter& mainTextureParam = updater[IShader::BindInput::MAINTEXTURE];
		if (mainTextureParam) {
			size_t k = mainTextureParam.slot;
			IRender::Resource*& texture = drawCall.textureResources[k];
			texture = mainTexture()->GetRenderResource();
		}

		// instanced data
		const PassBase::Parameter& inTexCoordRectParam = updater[StaticBytes(subTexMark)];
		if (inTexCoordRectParam) {
			size_t k = inTexCoordRectParam.slot;
			IRender::Resource::DrawCallDescription::BufferRange& bufferRange = drawCall.bufferResources[k];
			bufferRange = batchInstancedDataComponent->Allocate(inTexCoordRect);
		}

		const PassBase::Parameter& outTexCoordRectParam = updater[StaticBytes(mainCoordRect)];
		if (outTexCoordRectParam) {
			size_t k = outTexCoordRectParam.slot;
			IRender::Resource::DrawCallDescription::BufferRange& bufferRange = drawCall.bufferResources[k];
			bufferRange = batchInstancedDataComponent->Allocate(outTexCoordRect);
		}
	}
}
