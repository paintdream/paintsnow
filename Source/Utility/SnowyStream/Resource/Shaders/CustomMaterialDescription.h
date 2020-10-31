// CustomMaterialParameterFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"
#include "../../../../General/Interface/IAsset.h"
#include "../../../../Core/System/Tiny.h"

namespace PaintsNow {
	class CustomMaterialDescription : public TReflected<CustomMaterialDescription, SharedTiny> {
	public:
		CustomMaterialDescription();

		void SetCode(const String& text);
		void SetInput(const String& category, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config);
		void SetComplete(Bytes& extUniformBuffer, Bytes& extOptionBuffer);
		void ReflectExternal(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer);

		void SyncOptions(Bytes& dstOptionBuffer, const CustomMaterialDescription& rhs, const Bytes& srcOptionBuffer);
		void ReflectVertexTemplate(IReflect& reflect, Bytes& uniformData, Bytes& optionData);
		void ReflectOptionTemplate(IReflect& reflect, Bytes& uniformData, Bytes& optionData);
		void ReflectUniformTemplate(IReflect& reflect, Bytes& uniformData, Bytes& optionData);
		void ReflectInputTemplate(IReflect& reflect, Bytes& uniformData, Bytes& optionData);
		void ReflectOutputTemplate(IReflect& reflect, Bytes& uniformData, Bytes& optionData);

		IShader::BindBuffer uniformBuffer;
		String code;

		enum {
			VAR_VERTEX,
			VAR_UNIFORM,
			VAR_OPTION,
			VAR_INPUT,
			VAR_OUTPUT
		};

		class Entry : public IAsset::Material::Variable {
		public:
			Entry() : var(0), schema(0), slot(0), offset(0) {}

			uint8_t var;
			uint8_t schema;
			uint16_t slot;
			uint32_t offset;
		};

		std::vector<Entry> entries;
		std::vector<IShader::BindTexture> uniformTextureBindings;
		std::vector<IShader::BindBuffer> vertexBufferBindings;

		CustomMaterialDescription* dependency;
		Bytes* dependOptBuffer;
	};
}


