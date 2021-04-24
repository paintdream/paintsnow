#include "StencilMaskRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

StencilMaskRenderStage::StencilMaskRenderStage(const String& config) : InputDepthStencil(renderTargetDescription.depthStorage), OutputMask(renderTargetDescription.depthStorage) {
	renderStateDescription.colorWrite = 1;
	renderStateDescription.depthTest = 0;
	renderStateDescription.stencilTest = IRender::Resource::RenderStateDescription::EQUAL;
	renderStateDescription.stencilWrite = 0;
	renderStateDescription.stencilValue = 0;
	renderStateDescription.stencilMask = 0xff;

	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
}

TObject<IReflect>& StencilMaskRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputDepthStencil);
		ReflectProperty(OutputMask);
	}

	return *this;
}

void StencilMaskRenderStage::Prepare(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	// Do nothing

	BaseClass::Prepare(engine, queue);
}

void StencilMaskRenderStage::Update(Engine& engine, IRender::Queue* queue) {
	ConstMapPass& Pass = GetPass();
	// TODO: Bind Box Mesh ...

	BaseClass::Update(engine, queue);
}
