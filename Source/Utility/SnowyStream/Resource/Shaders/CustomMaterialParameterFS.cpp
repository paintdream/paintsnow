#include "CustomMaterialParameterFS.h"

using namespace PaintsNow;

CustomShaderDescription::CustomShaderDescription() : textureCount(0) {}

CustomMaterialParameterFS::CustomMaterialParameterFS() {
	description = TShared<CustomShaderDescription>::From(new CustomShaderDescription());
}

String CustomMaterialParameterFS::GetShaderText() {
	return description->code;
}

static inline Unique UniqueFromVariableType(uint32_t id) {
	switch (id) {
	case IAsset::TYPE_BOOL:
		return UniqueType<bool>::Get();
	case IAsset::TYPE_FLOAT:
		return UniqueType<float>::Get();
	case IAsset::TYPE_FLOAT2:
		return UniqueType<Float2>::Get();
	case IAsset::TYPE_FLOAT3:
		return UniqueType<Float3>::Get();
	case IAsset::TYPE_FLOAT4:
		return UniqueType<Float4>::Get();
	case IAsset::TYPE_MATRIX3:
		return UniqueType<MatrixFloat3x3>::Get();
	case IAsset::TYPE_MATRIX4:
		return UniqueType<MatrixFloat3x3>::Get();
	case IAsset::TYPE_TEXTURE:
		return UniqueType<IRender::Resource*>::Get();
	}

	return UniqueType<Void>::Get();
}

class BufferMetaChain : public MetaChainBase {
public:
	BufferMetaChain(IShader::BindBuffer& buffer) : bindBuffer(buffer) {}
	virtual const MetaChainBase* GetNext() const {
		return nullptr;
	}

	virtual const MetaNodeBase* GetNode() const {
		return &bindBuffer;
	}

	virtual const MetaNodeBase* GetRawNode() const {
		return &bindBuffer;
	}

	IShader::BindBuffer& bindBuffer;
};

TObject<IReflect>& CustomMaterialParameterFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(uniformBuffer);

		std::vector<IAsset::Material::Variable>& variables = description->uniformTemplate.variables;
		uint32_t offset = 0;
		uint8_t* bufferBase = uniformBlock.GetData();

		for (size_t i = 0; i < variables.size(); i++) {
			IAsset::Material::Variable& var = variables[i];
			String name;
			name.assign((const char*)var.key.GetData(), var.key.GetSize());

			if (var.type == IAsset::TYPE_TEXTURE) {
				uint32_t index = var.Parse(UniqueType<IAsset::TextureIndex>()).index;
				assert(index < uniformTextureBindings.size());
				reflect.Property(uniformTextureBindings[index], UniqueType<BindTexture>::Get(), UniqueType<BindTexture>::Get(), name.c_str(), &uniformTextureBindings[0], &uniformTextureBindings[index], nullptr);
			} else {
				Unique type = UniqueFromVariableType(var.type);
				static IReflectObject dummy;
				BufferMetaChain chain(uniformBuffer);
				// Make alignment
				uint32_t size = safe_cast<uint32_t>(type->GetSize());
				offset = (offset + size - 1) & (size - 1); // make alignment for variable
				reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &chain);
				offset += safe_cast<uint32_t>(type->GetSize());
			}
		}
	}

	return *this;
}

void CustomMaterialParameterFS::UpdateUniforms() {
	uint32_t offset = 0;
	uint32_t textureCount = 0;
	std::vector<IAsset::Material::Variable>& variables = description->uniformTemplate.variables;
	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		if (var.type == IAsset::TYPE_TEXTURE) {
			textureCount++;
		} else {
			Unique type = UniqueFromVariableType(var.type);
			static IReflectObject dummy;
			// Make alignment
			uint32_t size = safe_cast<uint32_t>(type->GetSize());
			offset = (offset + size - 1) & (size - 1); // make alignment for variable
			offset += size;
		}
	}

	uniformBlock.Resize(offset);
	uniformTextureBindings.resize(textureCount);
}

void CustomMaterialParameterFS::SetCode(const String& text) {
	description->code = text;
}

void CustomMaterialParameterFS::SetInput(const String& type, const String& name, const std::vector<std::pair<String, String> >& config) {
	IAsset::Material::Variable var;
	var.key.Assign((const uint8_t*)name.c_str(), safe_cast<uint32_t>(name.size()));
	static String typeTexture = UniqueType<IAsset::TextureIndex>::Get()->GetBriefName();
	static String typeFloat = UniqueType<float>::Get()->GetBriefName();
	static String typeFloat2 = UniqueType<Float2>::Get()->GetBriefName();
	static String typeFloat3 = UniqueType<Float3>::Get()->GetBriefName();
	static String typeFloat4 = UniqueType<Float4>::Get()->GetBriefName();

	if (type == typeTexture) {
		var = IAsset::TextureIndex(description->textureCount++);
	} else if (type == typeFloat) {
		var = float(0);
	} else if (type == typeFloat2) {
		var = Float2(0.0f, 0.0f);
	} else if (type == typeFloat3) {
		var = Float3(0.0f, 0.0f, 0.0f);
	} else if (type == typeFloat4) {
		var = Float4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	
	description->uniformTemplate.variables.emplace_back(std::move(var));
}

void CustomMaterialParameterFS::SetComplete() {
	UpdateUniforms();
}
