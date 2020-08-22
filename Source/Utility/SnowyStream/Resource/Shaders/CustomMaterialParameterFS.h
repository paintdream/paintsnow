// CustomMaterialParameterFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __CUSTOMMATERIALPARAMETER_FS_H
#define __CUSTOMMATERIALPARAMETER_FS_H

#include "../../../../General/Interface/IShader.h"
#include "../../../../General/Interface/IAsset.h"
#include "../../../../Core/System/Tiny.h"

namespace PaintsNow {
	class CustomShaderDescription : public TReflected<CustomShaderDescription, SharedTiny> {
	public:
		CustomShaderDescription();

		IAsset::Material uniformTemplate;
		String code;
		uint32_t textureCount;
	};

	class CustomMaterialParameterFS : public TReflected<CustomMaterialParameterFS, IShader> {
	public:
		CustomMaterialParameterFS();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual String GetShaderText() override;
		void UpdateUniforms();
		void SetCode(const String& text);
		void SetInput(const String& type, const String& name, const std::vector<std::pair<String, String> >& config);
		void SetComplete();

		TShared<CustomShaderDescription> description;
		std::vector<IShader::BindTexture> uniformTextureBindings;
		BindBuffer uniformBuffer;
		Bytes uniformBlock;
	};
}

#endif // __CUSTOMMATERIALPARAMETER_FS_H
