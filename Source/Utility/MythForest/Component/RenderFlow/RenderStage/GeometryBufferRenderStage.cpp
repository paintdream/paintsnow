#include "GeometryBufferRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

GeometryBufferRenderStage::GeometryBufferRenderStage() : BaseClass(2),
	BaseColorOcclusion(renderTargetDescription.colorBufferStorages[0]),
	NormalRoughnessMetallic(renderTargetDescription.colorBufferStorages[1]),
	Depth(renderTargetDescription.depthStencilStorage) {
	clearDescription.clearColorBit = IRender::Resource::ClearDescription::CLEAR;
	clearDescription.clearDepthBit = IRender::Resource::ClearDescription::CLEAR;
	clearDescription.clearStencilBit = IRender::Resource::ClearDescription::CLEAR;

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

void GeometryBufferRenderStage::PrepareResources(Engine& engine) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;

	BaseColorOcclusion.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &BaseColorOcclusion), false, 0, nullptr);
	BaseColorOcclusion.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	BaseColorOcclusion.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;

	NormalRoughnessMetallic.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &NormalRoughnessMetallic), false, 0, nullptr);
	NormalRoughnessMetallic.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	NormalRoughnessMetallic.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;

	Depth.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &Depth), false, 0, nullptr);
	Depth.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::FLOAT;
	Depth.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::DEPTH;

	BaseClass::PrepareResources(engine);
}
