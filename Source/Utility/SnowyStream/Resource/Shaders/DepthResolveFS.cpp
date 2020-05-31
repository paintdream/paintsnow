#define USE_SWIZZLE
#include "DepthResolveFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

DepthResolveFS::DepthResolveFS() {
	depthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String DepthResolveFS::GetShaderText() {
	return UnifyShaderCode(
		float depth = texture(depthTexture, rasterCoord.xy).x;
		outputDepth.x = resolveParam.x / (resolveParam.y + resolveParam.z * depth);
	);
}

TObject<IReflect>& DepthResolveFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// inputs
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(resolveParam)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(outputDepth)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
