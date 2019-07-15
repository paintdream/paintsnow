#include "WidgetComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

WidgetComponent::WidgetComponent(MeshResource& model) : quadMesh(model) {}

uint32_t WidgetComponent::CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) {
	/*
	uint32_t orgCount = safe_cast<uint32_t>(outputDrawCalls.size());
	uint32_t count = quadMesh.CollectDrawCalls(outputDrawCalls, inputRenderData);
	for (uint32_t i = 0; i < count; i++) {
		OutputRenderData& renderData = outputDrawCalls[i];
		IRender::Resource::DrawCallDescription& drawCall = renderData.drawCallDescription;
		IRender::Resource::RenderStateDescription& renderState = renderData.renderStateDescription;

		// TODO:
	}*/

	return 0;
}
