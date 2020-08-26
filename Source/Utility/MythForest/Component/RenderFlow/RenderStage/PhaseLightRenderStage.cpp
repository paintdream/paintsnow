#include "PhaseLightRenderStage.h"

using namespace PaintsNow;

PhaseLightRenderStage::PhaseLightRenderStage(const String& s) : OutputColor(renderTargetDescription.colorBufferStorages[0]) {
	renderStateDescription.blend = 1;
	renderTargetDescription.colorBufferStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorBufferStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
	renderTargetDescription.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
}

TObject<IReflect>& PhaseLightRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);
		ReflectProperty(InputColor);
		ReflectProperty(Depth);
		ReflectProperty(BaseColorOcclusion);
		ReflectProperty(NormalRoughnessMetallic);
		ReflectProperty(PhaseLightView);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void PhaseLightRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	BaseClass::PrepareResources(engine, queue);
}

void PhaseLightRenderStage::Tick(Engine& engine, IRender::Queue* queue) {
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);
	RenderStage::Tick(engine, queue);
}

void PhaseLightRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	const std::vector<RenderPortPhaseLightView::PhaseInfo>& phases = PhaseLightView.phases;
	MultiHashTracePass& pass = GetPass();
	MultiHashTraceFS& fs = pass.shaderMultiHashTrace;
	fs.dstDepthTexture.resource = Depth.textureResource->GetTexture();
	fs.dstBaseColorOcclusionTexture.resource = BaseColorOcclusion.textureResource->GetTexture();
	fs.dstNormalRoughnessMetallicTexture.resource = NormalRoughnessMetallic.textureResource->GetTexture();
	fs.dstInverseProjection = CameraView->inverseProjectionMatrix;

	for (size_t i = 0; i < phases.size(); i++) {
		const RenderPortPhaseLightView::PhaseInfo& phase = phases[i];
		// Set params
		fs.srcProjection = CameraView->inverseViewMatrix * phase.viewProjectionMatrix;
		fs.srcInverseProjection = Math::Inverse(fs.srcProjection);
		UShort3& dim = phase.irradiance->description.dimension;
		Float2 inv(1.0f / dim.x(), 1.0f / dim.y());

		// generate offsets
		for (size_t i = 0; i < fs.offsets.size(); i++) {
			fs.offsets[i] = Float2((float)rand() / RAND_MAX * inv.x(), (float)rand() / RAND_MAX * inv.y());
		}

		BaseClass::UpdatePass(engine, queue);
	}
}
