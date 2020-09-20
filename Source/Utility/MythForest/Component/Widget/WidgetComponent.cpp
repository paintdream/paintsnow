#include "WidgetComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

using namespace PaintsNow;

WidgetComponent::WidgetComponent(TShared<MeshResource> mesh, TShared<BatchComponent> batch, const TShared<BatchComponent>& batchInstanced) : BaseClass(mesh, batch) {
	batchInstancedDataComponent = batchInstanced;
	Flag().fetch_or(RENDERABLECOMPONENT_CAMERAVIEW, std::memory_order_acquire);
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
			drawCall.textureResources[mainTextureParam.slot] = mainTexture()->GetRenderResource();
		}

		// instanced data
		const PassBase::Parameter& inTexCoordRectParam = updater[StaticBytes(subTexMark)];
		if (inTexCoordRectParam) {
			drawCall.bufferResources[inTexCoordRectParam.slot] = batchInstancedDataComponent->Allocate(inTexCoordRect);
		}

		const PassBase::Parameter& outTexCoordRectParam = updater[StaticBytes(mainCoordRect)];
		if (outTexCoordRectParam) {
			drawCall.bufferResources[outTexCoordRectParam.slot] = batchInstancedDataComponent->Allocate(outTexCoordRect);
		}
	}
}
