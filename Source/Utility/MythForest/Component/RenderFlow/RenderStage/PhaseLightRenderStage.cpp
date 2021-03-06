#include "PhaseLightRenderStage.h"

using namespace PaintsNow;

PhaseLightRenderStage::PhaseLightRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]) {
	renderStateDescription.blend = 1;
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
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

void PhaseLightRenderStage::Prepare(Engine& engine, IRender::Queue* queue) {
	BaseClass::Prepare(engine, queue);
}

void PhaseLightRenderStage::Tick(Engine& engine, IRender::Queue* queue) {
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	BaseClass::Tick(engine, queue);
}

void PhaseLightRenderStage::Update(Engine& engine, IRender::Queue* queue) {
	const std::vector<RenderPortPhaseLightView::PhaseInfo>& phases = PhaseLightView.phases;
	MultiHashTracePass& pass = GetPass();
	MultiHashTraceFS& fs = pass.shaderMultiHashTrace;
	fs.dstDepthTexture.resource = Depth.textureResource->GetRenderResource();
	fs.dstBaseColorOcclusionTexture.resource = BaseColorOcclusion.textureResource->GetRenderResource();
	fs.dstNormalRoughnessMetallicTexture.resource = NormalRoughnessMetallic.textureResource->GetRenderResource();
	fs.dstInverseProjection = CameraView->inverseProjectionMatrix;

	for (size_t i = 0; i < phases.size(); i++) {
		const RenderPortPhaseLightView::PhaseInfo& phase = phases[i];
		// Set params
		fs.srcProjection = CameraView->inverseViewMatrix * phase.viewMatrix * phase.projectionMatrix;
		fs.srcInverseProjection = Math::Inverse(fs.srcProjection);
		UShort3& dim = phase.irradiance->description.dimension;
		Float2 inv(1.0f / dim.x(), 1.0f / dim.y());

		// generate offsets
		for (size_t i = 0; i < fs.offsets.size(); i++) {
			fs.offsets[i] = Float2((float)rand() / RAND_MAX * inv.x(), (float)rand() / RAND_MAX * inv.y());
		}

		BaseClass::Update(engine, queue);
	}
}
