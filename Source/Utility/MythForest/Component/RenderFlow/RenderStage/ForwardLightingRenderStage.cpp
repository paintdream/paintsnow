#include "ForwardLightingRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

ForwardLightingRenderStage::ForwardLightingRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]), InputColor(renderTargetDescription.colorStorages[0]), InputDepth(renderTargetDescription.depthStorage), OutputDepth(renderTargetDescription.depthStorage) {
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	renderStateDescription.depthTest = IRender::Resource::RenderStateDescription::GREATER;
	renderStateDescription.depthWrite = 1;
}

TObject<IReflect>& ForwardLightingRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(LightSource);
		ReflectProperty(Primitives);

		ReflectProperty(InputColor);
		ReflectProperty(InputDepth);
		ReflectProperty(OutputColor);
		ReflectProperty(OutputDepth);
	}

	return *this;
}

void ForwardLightingRenderStage::Prepare(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	OutputDepth.renderTargetDescription.state.format = IRender::Resource::TextureDescription::FLOAT;
	OutputDepth.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::DEPTH_STENCIL;
	OutputDepth.renderTargetDescription.state.immutable = false;
	OutputDepth.renderTargetDescription.state.attachment = true;

	BaseClass::Prepare(engine, queue);
}

void ForwardLightingRenderStage::Update(Engine& engine, IRender::Queue* queue) {
	BaseClass::Update(engine, queue);
}
