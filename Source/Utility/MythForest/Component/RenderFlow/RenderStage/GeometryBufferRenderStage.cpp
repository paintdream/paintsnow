#include "GeometryBufferRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

GeometryBufferRenderStage::GeometryBufferRenderStage(const String& s) : BaseClass(2),
	BaseColorOcclusion(renderTargetDescription.colorBufferStorages[0]),
	NormalRoughnessMetallic(renderTargetDescription.colorBufferStorages[1]),
	Depth(renderTargetDescription.depthStencilStorage) {
	renderTargetDescription.colorBufferStorages[0].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.colorBufferStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorBufferStorages[1].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.colorBufferStorages[1].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStencilStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.depthStencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

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

void GeometryBufferRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;

	BaseColorOcclusion.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &BaseColorOcclusion), false, 0, nullptr);
	BaseColorOcclusion.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	BaseColorOcclusion.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	BaseColorOcclusion.renderTargetTextureResource->description.state.immutable = false;
	BaseColorOcclusion.renderTargetTextureResource->description.state.attachment = true;

	NormalRoughnessMetallic.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &NormalRoughnessMetallic), false, 0, nullptr);
	NormalRoughnessMetallic.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	NormalRoughnessMetallic.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	NormalRoughnessMetallic.renderTargetTextureResource->description.state.immutable = false;
	NormalRoughnessMetallic.renderTargetTextureResource->description.state.attachment = true;

	Depth.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &Depth), false, 0, nullptr);
	Depth.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::FLOAT;
	Depth.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::DEPTH_STENCIL;
	Depth.renderTargetTextureResource->description.state.immutable = false;
	Depth.renderTargetTextureResource->description.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}
