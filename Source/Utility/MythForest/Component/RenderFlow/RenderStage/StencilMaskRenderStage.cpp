#include "StencilMaskRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

StencilMaskRenderStage::StencilMaskRenderStage(const String& config) : InputDepth(renderTargetDescription.depthStorage), InputColorPlaceHolder(renderTargetDescription.colorStorages[0]), OutputDepth(renderTargetDescription.depthStorage) {
	renderStateDescription.colorWrite = 0;
	renderStateDescription.depthTest = 1;
	renderStateDescription.stencilTest = IRender::Resource::RenderStateDescription::ALWAYS;
	renderStateDescription.stencilWrite = 1;
	renderStateDescription.stencilValue = atoi(config.c_str());
	renderStateDescription.stencilMask = 0xff;

	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
	renderTargetDescription.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
}

TObject<IReflect>& StencilMaskRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputDepth);
		ReflectProperty(InputColorPlaceHolder);
		ReflectProperty(OutputDepth);
	}

	return *this;
}

void StencilMaskRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	// Do nothing

	BaseClass::PrepareResources(engine, queue);
}

void StencilMaskRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	ConstMapPass& Pass = GetPass();
	// TODO: Bind Box Mesh ...

	BaseClass::UpdatePass(engine, queue);
}
