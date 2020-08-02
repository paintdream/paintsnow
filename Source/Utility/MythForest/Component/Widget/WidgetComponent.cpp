#include "WidgetComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

WidgetComponent::WidgetComponent(MeshResource& model, TShared<MaterialResource> material) : quadMesh(model), materialResource(material), mainTextureSlotInMaterial(0) {
	PassBase::Parameter& mainTextureParam = material->mutationShaderResource->GetPassUpdater()[IShader::BindInput::MAINTEXTURE];
	assert(mainTextureParam);

	mainTextureSlotInMaterial = mainTextureParam.slot;
}

uint32_t WidgetComponent::CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) {
	if (mainTexture() == nullptr) return 0;

	uint32_t orgCount = safe_cast<uint32_t>(outputDrawCalls.size());
	materialResource->CollectDrawCalls(outputDrawCalls, inputRenderData);
	for (uint32_t i = 0; i < safe_cast<uint32_t>(outputDrawCalls.size()); i++) {
		OutputRenderData& renderData = outputDrawCalls[i];
		IRender::Resource::DrawCallDescription& drawCall = renderData.drawCallDescription;
		// IRender::Resource::RenderStateDescription& renderState = renderData.renderStateDescription;
		drawCall.textureResources[mainTextureSlotInMaterial] = mainTexture()->GetTexture();
	}

	return safe_cast<uint32_t>(outputDrawCalls.size()) - orgCount;
}
