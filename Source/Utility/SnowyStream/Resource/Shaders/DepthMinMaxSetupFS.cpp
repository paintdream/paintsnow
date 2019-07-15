#define USE_SWIZZLE
#include "DepthMinMaxSetupFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

DepthMinMaxSetupFS::DepthMinMaxSetupFS() {
	depthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String DepthMinMaxSetupFS::GetShaderText() {
	return UnifyShaderCode(
		float depth0 = texture(depthTexture, rasterCoord.xy).x;
		float depth1 = texture(depthTexture, rasterCoord.xy + float2(invScreenSize.x, 0)).x;
		float depth2 = texture(depthTexture, rasterCoord.xy + float2(0, invScreenSize.y)).x;
		float depth3 = texture(depthTexture, rasterCoord.xy + float2(invScreenSize.x, invScreenSize.y)).x;
	
		outputDepth.x = min(min(depth0, depth1), min(depth2, depth3));
		outputDepth.y = max(max(depth0, depth1), max(depth2, depth3));
	);
}

TObject<IReflect>& DepthMinMaxSetupFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// inputs
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);
		ReflectProperty(rasterCoord)[IShader::BindInput(IShader::BindInput::TEXCOORD)];
		ReflectProperty(invScreenSize)[uniformBuffer][IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(outputDepth)[IShader::BindOutput(IShader::BindOutput::COLOR)];
	}

	return *this;
}
