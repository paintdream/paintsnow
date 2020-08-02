#include "WidgetComponent.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

WidgetComponent::WidgetComponent(MeshResource& model, TShared<MaterialResource> material, TShared<BatchComponent> batch) : quadMesh(model), materialResource(material), batchComponent(batch) {}

void WidgetComponent::Initialize(Engine& engine, Entity* entity) {
	batchComponent->InstanceInitialize(engine);
}

void WidgetComponent::Uninitialize(Engine& engine, Entity* entity) {
	batchComponent->InstanceUninitialize(engine);
}

Bytes WidgetComponent::EncodeTexCoordParams() {
	Bytes data;
	data.Resize(3 * sizeof(Float4));

	Float4* p = reinterpret_cast<Float4*>(data.GetData());
	p[0] = outTexCoordRect;

	// TODO: finish the remaining params.
	return data;
}

void WidgetComponent::GenerateDrawCall() {
	if (!(renderData.shaderResource)) {
		std::vector<IDrawCallProvider::OutputRenderData> drawCalls;
		static IDrawCallProvider::InputRenderData inputRenderData(0.0f, nullptr);
		uint32_t count = materialResource->CollectDrawCalls(drawCalls, inputRenderData);
		assert(count == 1);
		renderData = std::move(drawCalls[0]);

		// fill instance data
		PassBase::Updater& updater = renderData.shaderResource->GetPassUpdater();
		PassBase::Parameter& texCoordRectParam = updater[PassBase::Updater::MakeKeyFromString("texCoordRect")];
		PassBase::Parameter& texCoordMaskParam = updater[PassBase::Updater::MakeKeyFromString("texCoordMask")];
		PassBase::Parameter& texCoordScaleParam = updater[PassBase::Updater::MakeKeyFromString("texCoordScale")];

		assert(texCoordRectParam.slot == texCoordMaskParam.slot && texCoordRectParam.slot == texCoordScaleParam.slot);

		// Allocate buffer
		Bytes data = EncodeTexCoordParams();
		IRender::Resource::DrawCallDescription::BufferRange bufferRange = batchComponent->Allocate(data);
		renderData.drawCallDescription.bufferResources[texCoordRectParam.slot] = bufferRange;
	}
	
	PassBase::Updater& updater = renderData.shaderResource->GetPassUpdater();
	PassBase::Parameter& mainTextureParam = updater[IShader::BindInput::MAINTEXTURE];
	assert(mainTextureParam);
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
