#include "ShadowMaskRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ShadowMaskRenderStage::ShadowMaskRenderStage(const String& config) : OutputMask(renderTargetDescription.colorBufferStorages[0]) {}

TObject<IReflect>& ShadowMaskRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(LightSource);
		ReflectProperty(InputDepth);
		ReflectProperty(OutputMask);
	}

	return *this;
}

void ShadowMaskRenderStage::PrepareResources(Engine& engine) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputMask.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &OutputMask), false, 0, nullptr);
	OutputMask.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	OutputMask.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::R;
	OutputMask.renderTargetTextureResource->description.state.immutable = false;

	BaseClass::PrepareResources(engine);
}

void ShadowMaskRenderStage::UpdatePass(Engine& engine) {
	ShadowMaskPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	ShadowMaskFS& mask = Pass.mask;
	mask.depthTexture.resource = InputDepth.textureResource->GetTexture();

	BaseClass::UpdatePass(engine);
}
