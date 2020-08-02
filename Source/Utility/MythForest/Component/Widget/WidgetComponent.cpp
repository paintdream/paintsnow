#include "WidgetComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

WidgetComponent::WidgetComponent(MeshResource& model, TShared<MaterialResource> material, TShared<BatchComponent> batch) : quadMesh(model), materialResource(material), batchComponent(batch) {}

void WidgetComponent::GenerateDrawCall() {
	PassBase::Parameter& mainTextureParam = materialResource->mutationShaderResource->GetPassUpdater()[IShader::BindInput::MAINTEXTURE];
	assert(mainTextureParam);

	// TODO: fill params
	renderData.shaderResource = materialResource->mutationShaderResource;
	
	IRender::Resource::DrawCallDescription& drawCall = renderData.drawCallDescription;
	drawCall.textureResources[mainTextureParam.slot] = mainTexture()->GetTexture();
}

uint32_t WidgetComponent::CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) {
	if (mainTexture() == nullptr) return 0;
	
	if (!(renderData.shaderResource)) {
		GenerateDrawCall();
	}

	outputDrawCalls.emplace_back(renderData);
	return 1;
}
