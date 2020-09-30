// CustomMaterialParameterFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"
#include "../../../../General/Interface/IAsset.h"
#include "../../../../Core/System/Tiny.h"

namespace PaintsNow {
	class CustomShaderDescription : public TReflected<CustomShaderDescription, SharedTiny> {
	public:
		CustomShaderDescription();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void SetCode(const String& text);
		void SetInput(const String& type, const String& name, const std::vector<std::pair<String, String> >& config);
		void SetComplete();

		IAsset::Material uniformTemplate;
		std::vector<IShader::BindTexture> uniformTextureBindings;
		Bytes uniformBlock;
		String code;
		IShader::BindBuffer uniformBuffer;
		uint32_t textureCount;
	};
}


