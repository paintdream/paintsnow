#include "CustomMaterialParameterFS.h"

using namespace PaintsNow;

CustomShaderDescription::CustomShaderDescription() {}

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
	const MetaChainBase* GetNext() const override {
		return nullptr;
	}

	const MetaNodeBase* GetNode() const override {
		return &bindBuffer;
	}

	const MetaNodeBase* GetRawNode() const override {
		return &bindBuffer;
	}

	IShader::BindBuffer& bindBuffer;
};

void CustomShaderDescription::ReflectUniformTemplate(IReflect& reflect) {
	ReflectProperty(uniformBuffer);

	std::vector<IAsset::Material::Variable>& variables = uniformTemplate.variables;
	uint32_t offset = 0;
	uint8_t* bufferBase = uniformBlock.GetData();

	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		if (var.type == IAsset::TYPE_TEXTURE) {
			uint32_t index = var.Parse(UniqueType<IAsset::TextureIndex>()).index;
			assert(index < uniformTextureBindings.size());
			reflect.Property(uniformTextureBindings[index], UniqueType<IShader::BindTexture>::Get(), UniqueType<IShader::BindTexture>::Get(), name.c_str(), &uniformTextureBindings[0], &uniformTextureBindings[index], nullptr);
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

void CustomShaderDescription::ReflectVertexTemplate(IReflect& reflect) {
	std::vector<IAsset::Material::Variable>& variables = vertexTemplate.variables;
	uint32_t offset = 0;
	uint8_t* bufferBase = uniformBlock.GetData();

	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

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

void CustomShaderDescription::ReflectInputTemplate(IReflect& reflect) {
	std::vector<IAsset::Material::Variable>& variables = inputTemplate.variables;
	uint32_t offset = 0;
	uint8_t* bufferBase = uniformBlock.GetData();

	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

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

void CustomShaderDescription::ReflectOutputTemplate(IReflect& reflect) {
	std::vector<IAsset::Material::Variable>& variables = inputTemplate.variables;
	uint32_t offset = 0;
	uint8_t* bufferBase = uniformBlock.GetData();

	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

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

TObject<IReflect>& CustomShaderDescription::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectUniformTemplate(reflect);
		ReflectInputTemplate(reflect);
		ReflectOutputTemplate(reflect);
	}

	return *this;
}

void CustomShaderDescription::SetComplete() {
	uint32_t offset = 0;
	std::vector<IAsset::Material::Variable>& variables = uniformTemplate.variables;
	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		if (var.type != IAsset::TYPE_TEXTURE && var.type != IAsset::TYPE_BOOL) {
			Unique type = UniqueFromVariableType(var.type);
			static IReflectObject dummy;
			// Make alignment
			uint32_t size = safe_cast<uint32_t>(type->GetSize());
			offset = (offset + size - 1) & (size - 1); // make alignment for variable
			offset += size;
		}
	}

	uniformBlock.Resize(offset);
}

void CustomShaderDescription::SetCode(const String& text) {
	code = text;
}

void CustomShaderDescription::SetInput(const String& category, const String& type, const String& name, const String& value, const std::vector<std::pair<String, String> >& config) {
	IAsset::Material::Variable var;
	var.key.Assign((const uint8_t*)name.c_str(), safe_cast<uint32_t>(name.size()));
	static String typeTexture = UniqueType<IAsset::TextureIndex>::Get()->GetBriefName();
	static String typeFloat = UniqueType<float>::Get()->GetBriefName();
	static String typeFloat2 = UniqueType<Float2>::Get()->GetBriefName();
	static String typeFloat3 = UniqueType<Float3>::Get()->GetBriefName();
	static String typeFloat4 = UniqueType<Float4>::Get()->GetBriefName();
	static String typeBool = UniqueType<bool>::Get()->GetBriefName();

	if (type == typeTexture) {
		var = IAsset::TextureIndex(safe_cast<uint32_t>(uniformTextureBindings.size()));
		uniformTextureBindings.emplace_back(IShader::BindTexture());
	} else if (type == typeFloat) {
		var = float(atof(value.c_str()));
	} else if (type == typeFloat2) {
		Float2 v(0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f", &v.x(), &v.y());
		var = v;
	} else if (type == typeFloat3) {
		Float3 v(0.0f, 0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f%f", &v.x(), &v.y(), &v.z());
		var = v;
	} else if (type == typeFloat4) {
		Float4 v(0.0f, 0.0f, 0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f%f%f", &v.x(), &v.y(), &v.z(), &v.w());
		var = v;
	} else if (type == typeBool) {
		var = value != "false";
	}

	IAsset::Material* t = &uniformTemplate;
	if (category == "Vertex" || category == "Instance") {
		t = &vertexTemplate;
		IShader::BindBuffer bindBuffer;
		bindBuffer.description.usage = category == "Vertex" ? IRender::Resource::BufferDescription::VERTEX : IRender::Resource::BufferDescription::INSTANCED;
		vertexBufferBindings.emplace_back(std::move(bindBuffer));
	} else if (category == "Uniform") {
		t = &uniformTemplate;
	} else if (category == "Input") {
		t = &inputTemplate;
	} else if (category == "Output") {
		t = &outputTemplate;
	}

	t->variables.emplace_back(std::move(var));
}

