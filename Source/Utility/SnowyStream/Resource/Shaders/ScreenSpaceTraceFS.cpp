#define USE_SWIZZLE
#include "ScreenSpaceTraceFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

ScreenSpaceTraceFS::ScreenSpaceTraceFS() {
}

String ScreenSpaceTraceFS::GetShaderText() {
	return UnifyShaderCode(
		float4 r = float4(0, 0, 0, 1);
		r.xyz = worldPosition.xyz;
		float4 startPosition = mult_vec(viewProjectionMatrix, r);
		r.xyz += traceDirection.xyz;
		float4 endPosition = mult_vec(viewProjectionMatrix, r);

		for (int i = 0; i < 12; i++) {
			// TODO: choose a screen-space division
			float4 pos = lerp(startPosition, endPosition, float(0.5));
			float3 ndc = pos.xyz / pos.w * float(0.5) + float3(0.5, 0.5, 0.5);
			float d = texture(Depth, ndc.xy).x - ndc.z;
			/// reverse-z d > 0 means near than probe line
			if (d > 0) {
				endPosition = pos;
			} else {
				startPosition = pos;
			}
		}

		float4 pos = lerp(startPosition, endPosition, float(0.5));
		traceCoord.xy = pos.xy / pos.w;
	);
}

TObject<IReflect>& ScreenSpaceTraceFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Depth);
		ReflectProperty(traceBuffer);

		ReflectProperty(viewProjectionMatrix)[traceBuffer];
		ReflectProperty(invScreenSize)[traceBuffer];

		ReflectProperty(worldPosition)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(traceDirection)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(rasterCoord)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(traceCoord)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
	}

	return *this;
}
