#include "GeometryBufferRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

GeometryBufferRenderStage::GeometryBufferRenderStage(const String& s) : BaseClass(2),
	BaseColorOcclusion(renderTargetDescription.colorStorages[0]),
	NormalRoughnessMetallic(renderTargetDescription.colorStorages[1]),
	Depth(renderTargetDescription.depthStorage) {
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[1].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.colorStorages[1].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	renderStateDescription.depthTest = IRender::Resource::RenderStateDescription::GREATER;
	renderStateDescription.depthWrite = 1;
	renderStateDescription.stencilWrite = 1;
}

TObject<IReflect>& GeometryBufferRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);
		ReflectProperty(Primitives);
		ReflectProperty(BaseColorOcclusion);
		ReflectProperty(NormalRoughnessMetallic);
		ReflectProperty(Depth);
	}

	return *this;
}

void GeometryBufferRenderStage::Prepare(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;

	BaseColorOcclusion.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	BaseColorOcclusion.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	BaseColorOcclusion.renderTargetDescription.state.immutable = false;
	BaseColorOcclusion.renderTargetDescription.state.attachment = true;

	NormalRoughnessMetallic.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	NormalRoughnessMetallic.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	NormalRoughnessMetallic.renderTargetDescription.state.immutable = false;
	NormalRoughnessMetallic.renderTargetDescription.state.attachment = true;

	Depth.renderTargetDescription.state.format = IRender::Resource::TextureDescription::FLOAT;
	Depth.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::DEPTH_STENCIL;
	Depth.renderTargetDescription.state.immutable = false;
	Depth.renderTargetDescription.state.attachment = true;

	BaseClass::Prepare(engine, queue);
}
