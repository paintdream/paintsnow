#include "CustomMaterialParameterFS.h"

using namespace PaintsNow;

CustomShaderDescription::CustomShaderDescription() {}

struct Schema {
	Schema(uint32_t s) { schema = s; }

	union {
		uint32_t schema;

		struct {
			uint16_t depend;
			uint16_t bind;
		};
	};
};

static inline Unique UniqueFromVariableType(uint32_t id) {
	switch (id) {
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
	default:
		assert(false);
		return UniqueType<float>::Get();
	}
}

static uint32_t SchemaFromVariableBinding(const String& binding, bool input) {
	if (binding == "POSITION") {
		if (input) {
			return IShader::BindInput::POSITION;
		}
	} else if (binding == "NORMAL") {
		if (input) {
			return IShader::BindInput::NORMAL;
		}
	} else if (binding == "BINORMAL") {
		if (input) {
			return IShader::BindInput::BINORMAL;
		}
	} else if (binding == "TANGENT") {
		if (input) {
			return IShader::BindInput::TANGENT;
		}
	} else if (binding == "COLOR") {
		if (input) {
			return IShader::BindInput::COLOR;
		} else {
			return IShader::BindOutput::COLOR;
		}
	} else if (binding == "COLOR_INSTANCED") {
		if (input) {
			return IShader::BindInput::COLOR_INSTANCED;
		}
	} else if (binding == "BONE_INDEX") {
		if (input) {
			return IShader::BindInput::BONE_INDEX;
		}
	} else if (binding == "BONE_WEIGHT") {
		if (input) {
			return IShader::BindInput::BONE_WEIGHT;
		}
	} else if (binding.compare(0, 8, "TEXCOORD") == 0) {
		uint32_t index = atoi(binding.c_str() + 8);
		if (input) {
			return IShader::BindInput::TEXCOORD + index;
		} else {
			return IShader::BindOutput::TEXCOORD + index;
		}
	}

	return ~(uint32_t)0;
}

template <class T>
class DummyMetaChain : public MetaChainBase {
public:
	DummyMetaChain(T& meta, const MetaChainBase* n = nullptr) : binder(meta), next(n) {}
	const MetaChainBase* GetNext() const override {
		return next;
	}

	const MetaNodeBase* GetNode() const override {
		return &binder;
	}

	const MetaNodeBase* GetRawNode() const override {
		return &binder;
	}

	T& binder;
	const MetaChainBase* next;
};

void CustomShaderDescription::ReflectUniformTemplate(IReflect& reflect, Bytes& data) {
	ReflectProperty(uniformBuffer);

	std::vector<IAsset::Material::Variable>& variables = uniformTemplate.variables;
	uint32_t offset = 0;
	uint8_t* bufferBase = data.GetData();

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
			DummyMetaChain<IShader::BindBuffer> chain(uniformBuffer);
			// Make alignment
			uint32_t size = safe_cast<uint32_t>(type->GetSize());
			offset = (offset + size - 1) & (size - 1); // make alignment for variable
			reflect.Property(dummy, type, type, name.c_str(), bufferBase + offset, bufferBase + offset + size, &chain);
			offset += size;
		}
	}
}

void CustomShaderDescription::ReflectVertexTemplate(IReflect& reflect) {
	std::vector<IAsset::Material::Variable>& variables = vertexTemplate.variables;

	assert(variables.size() == vertexBufferBindings.size());
	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromVariableType(var.type);
		static IReflectObject dummy;
		DummyMetaChain<IShader::BindBuffer> chain(vertexBufferBindings[i]);
		reflect.Property(vertexBufferBindings[i], UniqueType<IShader::BindBuffer>::Get(), UniqueType<IShader::BindBuffer>::Get(), name.c_str(), &vertexBufferBindings[0], &vertexBufferBindings[i], nullptr);
		reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &chain); // use var.value.GetData() to provide default value
	}
}

void CustomShaderDescription::ReflectOptionTemplate(IReflect& reflect, Bytes& data) {
	std::vector<IAsset::Material::Variable>& variables = optionTemplate.variables;
	uint8_t* bufferBase = data.GetData();
	uint32_t offset = 0;

	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		// extra data
		static IReflectObject dummy;
		// Make alignment
		uint32_t size = safe_cast<uint32_t>(var.value.GetSize());
		offset = (offset + size - 1) & (size - 1); // make alignment for variable

		if (size == 1) {
			IShader::BindConst<bool> slot((bool&)data[offset]);
			DummyMetaChain<IShader::BindConst<bool> > bindOption(slot);
			Unique type = UniqueType<bool>::Get();
			reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &bindOption);
		} else { // int
			assert(size == sizeof(uint32_t));
			IShader::BindConst<uint32_t> slot(*(uint32_t*)data[offset]);
			DummyMetaChain<IShader::BindConst<uint32_t> > bindOption(slot);
			Unique type = UniqueType<uint32_t>::Get();
			reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &bindOption);
		}

		offset += size;
	}
}

void CustomShaderDescription::ReflectInputTemplate(IReflect& reflect) {
	std::vector<IAsset::Material::Variable>& variables = inputTemplate.variables;

	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromVariableType(var.type);
		static IReflectObject dummy;
		Schema schema(var.schema);
		IShader::BindInput slot(schema.bind);
		DummyMetaChain<IShader::BindInput> bindInput(slot);
		reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), var.schema == ~(uint32_t)0 ? nullptr : &bindInput);
	}
}

void CustomShaderDescription::ReflectOutputTemplate(IReflect& reflect) {
	std::vector<IAsset::Material::Variable>& variables = inputTemplate.variables;

	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromVariableType(var.type);
		static IReflectObject dummy;
		IShader::BindOutput slot(var.schema);
		DummyMetaChain<IShader::BindOutput> bindOutput(slot);
		reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), var.schema == ~(uint32_t)0 ? nullptr : &bindOutput);
	}
}

void CustomShaderDescription::ReflectExternal(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	if (reflect.IsReflectProperty()) {
		ReflectUniformTemplate(reflect, extUniformBuffer);
		ReflectOptionTemplate(reflect, extOptionBuffer);
		ReflectVertexTemplate(reflect);
		ReflectInputTemplate(reflect);
		ReflectOutputTemplate(reflect);
	}
}

void CustomShaderDescription::SetComplete(Bytes& uniformBufferData, Bytes& optionBufferData) {
	size_t offset = uniformBufferData.GetSize();
	std::vector<IAsset::Material::Variable>& variables = uniformTemplate.variables;
	for (size_t i = 0; i < variables.size(); i++) {
		IAsset::Material::Variable& var = variables[i];
		if (var.type != IAsset::TYPE_TEXTURE) {
			Unique type = UniqueFromVariableType(var.type);
			static IReflectObject dummy;
			// Make alignment
			size_t size = safe_cast<uint32_t>(type->GetSize());
			offset = (offset + size - 1) & (size - 1); // make alignment for variable
			uniformBufferData.Resize(offset + size);
			memcpy(uniformBufferData.GetData() + offset, var.value.GetData(), size);
			offset += size;
		}
	}

	std::vector<IAsset::Material::Variable>& consts = optionTemplate.variables;
	offset = optionBufferData.GetSize();

	for (size_t j = 0; j < variables.size(); j++) {
		IAsset::Material::Variable& var = variables[j];

		if (var.GetUnique() == UniqueType<bool>::Get()) {
			optionBufferData.Append(var.value.GetData(), sizeof(bool));
			offset++;
		} else {
			assert(var.GetUnique() == UniqueType<int32_t>::Get());
			// padding
			const size_t size = sizeof(uint32_t);
			offset = (offset + size - 1) & (size - 1);

			uint32_t value = *reinterpret_cast<uint32_t*>(var.value.GetData());
			optionBufferData.Resize(offset + sizeof(value));
			*(uint32_t*)(optionBufferData.GetData() + offset) = value;
		}
	}
}

void CustomShaderDescription::SetCode(const String& text) {
	code = text;
}

void CustomShaderDescription::SetInput(const String& category, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {
	IAsset::Material::Variable var;
	var.key.Assign((const uint8_t*)name.c_str(), safe_cast<uint32_t>(name.size()));

	if (type == "texture") { // resource name
		var = IAsset::TextureIndex(safe_cast<uint32_t>(uniformTextureBindings.size()));
		// encode def path
		var.value.Assign((const uint8_t*)value.c_str(), value.size());
		uniformTextureBindings.emplace_back(IShader::BindTexture());
	} else if (type == "float") {
		var = float(atof(value.c_str()));
	} else if (type == "float2") {
		Float2 v(0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f", &v.x(), &v.y());
		var = v;
	} else if (type == "float3") {
		Float3 v(0.0f, 0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f%f", &v.x(), &v.y(), &v.z());
		var = v;
	} else if (type == "float4") {
		Float4 v(0.0f, 0.0f, 0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f%f%f", &v.x(), &v.y(), &v.z(), &v.w());
		var = v;
	} else if (type == "bool") {
		var = value != "false";
	} else if (type == "int") {
		var = (uint32_t)atoi(value.c_str()); // Hack: encode hash size
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
	} else if (category == "Option") {
		t = &optionTemplate;
	}

	var.schema = SchemaFromVariableBinding(binding, category == "Output");
	t->variables.emplace_back(std::move(var));
}

