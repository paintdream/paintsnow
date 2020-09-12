#include "WidgetComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

using namespace PaintsNow;

WidgetComponent::WidgetComponent(TShared<MeshResource> mesh, TShared<BatchComponent> batch, TShared<BatchComponent> batchInstanced) : BaseClass(mesh, batch) {
	batchInstancedDataComponent = batchInstanced;
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
		PassBase::Parameter& mainTextureParam = updater[IShader::BindInput::MAINTEXTURE];
		if (mainTextureParam) {
			drawCall.textureResources[mainTextureParam.slot] = mainTexture()->GetTexture();
		}

		// instanced data
		PassBase::Parameter& inTexCoordRectParam = updater[StaticBytes(subTexMark)];
		if (inTexCoordRectParam) {
			drawCall.bufferResources[inTexCoordRectParam.slot] = batchInstancedDataComponent->Allocate(inTexCoordRect);
		}

		PassBase::Parameter& outTexCoordRectParam = updater[StaticBytes(mainCoordRect)];
		if (outTexCoordRectParam) {
			drawCall.bufferResources[outTexCoordRectParam.slot] = batchInstancedDataComponent->Allocate(outTexCoordRect);
		}
	}
}
