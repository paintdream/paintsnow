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

static inline Unique UniqueFromEntryType(uint32_t id) {
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

static uint32_t SchemaFromPredefines(const String& binding, bool input) {
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

	return 0xFF;
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

void CustomShaderDescription::ReflectUniformTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	ReflectProperty(uniformBuffer);

	uint32_t offset = 0;
	uint8_t* bufferBase = extUniformBuffer.GetData();

	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_UNIFORM) continue;

		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		if (var.type == IAsset::TYPE_TEXTURE) {
			uint32_t index = var.Parse(UniqueType<IAsset::TextureIndex>()).index;
			assert(index < uniformTextureBindings.size());
			reflect.Property(uniformTextureBindings[index], UniqueType<IShader::BindTexture>::Get(), UniqueType<IShader::BindTexture>::Get(), name.c_str(), &uniformTextureBindings[0], &uniformTextureBindings[index], nullptr);
		} else {
			Unique type = UniqueFromEntryType(var.type);
			static IReflectObject dummy;
			DummyMetaChain<IShader::BindBuffer> chain(uniformBuffer);
			// Make alignment
			uint32_t size = safe_cast<uint32_t>(type->GetSize());
			offset = (offset + size - 1) & (size - 1); // make alignment for variable
			if (var.slot != 0xFF) { // depends
				IShader::BindEnable enable((bool&)extOptionBuffer[var.offset]);
				DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
				reflect.Property(dummy, type, type, name.c_str(), bufferBase + offset, bufferBase + offset + size, &enableChain);
			} else {
				reflect.Property(dummy, type, type, name.c_str(), bufferBase + offset, bufferBase + offset + size, &chain);
			}

			offset += size;
		}
	}
}

void CustomShaderDescription::ReflectVertexTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	for (size_t i = 0, j = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_VERTEX) continue;

		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromEntryType(var.type);
		static IReflectObject dummy;
		DummyMetaChain<IShader::BindBuffer> chain(vertexBufferBindings[j]);

		if (var.slot != 0xFF) { // depends
			IShader::BindEnable enable((bool&)extOptionBuffer[var.offset]);
			DummyMetaChain<IShader::BindEnable> enableChain(enable);
			reflect.Property(vertexBufferBindings[j], UniqueType<IShader::BindBuffer>::Get(), UniqueType<IShader::BindBuffer>::Get(), name.c_str(), &vertexBufferBindings[0], &vertexBufferBindings[j], &enableChain);
		} else {
			reflect.Property(vertexBufferBindings[j], UniqueType<IShader::BindBuffer>::Get(), UniqueType<IShader::BindBuffer>::Get(), name.c_str(), &vertexBufferBindings[0], &vertexBufferBindings[j], nullptr);
		}

		if (var.slot != 0xFF) { // depends
			IShader::BindEnable enable((bool&)extOptionBuffer[var.offset]);
			DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &enableChain); // use var.value.GetData() to provide default value
		} else {
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &chain); // use var.value.GetData() to provide default value
		}

		j++;
	}
}

void CustomShaderDescription::ReflectOptionTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	uint8_t* bufferBase = extOptionBuffer.GetData();
	uint32_t offset = 0;

	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_OPTION) continue;
		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		// extra data
		static IReflectObject dummy;
		// Make alignment
		uint32_t size = safe_cast<uint32_t>(var.value.GetSize());
		offset = (offset + size - 1) & (size - 1); // make alignment for variable

		if (size == 1) {
			IShader::BindConst<bool> slot((bool&)extOptionBuffer[offset]);
			DummyMetaChain<IShader::BindConst<bool> > chain(slot);
			Unique type = UniqueType<bool>::Get();

			if (var.slot != 0xFF) {
				IShader::BindEnable enable((bool&)extOptionBuffer[var.offset]);
				DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
				reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &enableChain);
			} else {
				reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &chain);
			}
		} else { // int
			assert(size == sizeof(uint32_t));
			IShader::BindConst<uint32_t> slot(*(uint32_t*)&extOptionBuffer[offset]);
			DummyMetaChain<IShader::BindConst<uint32_t> > chain(slot);
			Unique type = UniqueType<uint32_t>::Get();

			if (var.slot != 0xFF) {
				IShader::BindEnable enable((bool&)extOptionBuffer[var.offset]);
				DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
				reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &enableChain);
			} else {
				reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &chain);
			}
		}

		offset += size;
	}
}

void CustomShaderDescription::ReflectInputTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_INPUT) continue;

		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromEntryType(var.type);
		static IReflectObject dummy;
		Schema schema(var.schema);
		IShader::BindInput slot(schema.bind);
		DummyMetaChain<IShader::BindInput> chain(slot);

		if (var.slot != 0xFF) {
			IShader::BindEnable enable((bool&)extOptionBuffer[var.offset]);
			DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &enableChain);
		} else {
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &chain);
		}
	}
}

void CustomShaderDescription::ReflectOutputTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_OUTPUT) continue;

		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromEntryType(var.type);
		static IReflectObject dummy;
		IShader::BindOutput slot(var.schema);
		DummyMetaChain<IShader::BindOutput> chain(slot);

		if (var.slot != 0xFF) {
			IShader::BindEnable enable((bool&)extOptionBuffer[var.offset]);
			DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &enableChain);
		} else {
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &chain);
		}
	}
}

void CustomShaderDescription::ReflectExternal(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	if (reflect.IsReflectProperty()) {
		ReflectUniformTemplate(reflect, extUniformBuffer, extOptionBuffer);
		ReflectOptionTemplate(reflect, extUniformBuffer, extOptionBuffer);
		ReflectVertexTemplate(reflect, extUniformBuffer, extOptionBuffer);
		ReflectInputTemplate(reflect, extUniformBuffer, extOptionBuffer);
		ReflectOutputTemplate(reflect, extUniformBuffer, extOptionBuffer);
	}
}

void CustomShaderDescription::SetComplete(Bytes& uniformBufferData, Bytes& optionBufferData) {
	size_t uniformOffset = uniformBufferData.GetSize();
	size_t optionOffset = optionBufferData.GetSize();

	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var == VAR_UNIFORM) {
			if (var.type != IAsset::TYPE_TEXTURE) {
				Unique type = UniqueFromEntryType(var.type);
				static IReflectObject dummy;
				// Make alignment
				size_t size = safe_cast<uint32_t>(type->GetSize());
				uniformOffset = (uniformOffset + size - 1) & (size - 1); // make alignment for variable
				uniformBufferData.Resize(uniformOffset + size);
				memcpy(uniformBufferData.GetData() + uniformOffset, var.value.GetData(), size);

				var.offset = safe_cast<uint32_t>(uniformOffset);
				uniformOffset += size;
			}
		} else if (var.var == VAR_OPTION) {
			if (var.GetUnique() == UniqueType<bool>::Get()) {
				optionBufferData.Append(var.value.GetData(), sizeof(bool));
				optionOffset++;
			} else {
				assert(var.GetUnique() == UniqueType<int32_t>::Get());
				// padding
				const size_t size = sizeof(uint32_t);
				optionOffset = (optionOffset + size - 1) & (size - 1);

				uint32_t value = *reinterpret_cast<uint32_t*>(var.value.GetData());
				optionBufferData.Resize(optionOffset + sizeof(value));
				*(uint32_t*)(optionBufferData.GetData() + optionOffset) = value;

				var.offset = safe_cast<uint32_t>(optionOffset);
				optionOffset += sizeof(value);
			}
		}
	}
}

void CustomShaderDescription::SetCode(const String& text) {
	code = text;
}

void CustomShaderDescription::SetInput(const String& category, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {
	Entry var;

	if (type == "texture") { // resource name
		var.SetValue(IAsset::TextureIndex(safe_cast<uint32_t>(uniformTextureBindings.size())));
		// encode def path
		var.value.Assign((const uint8_t*)value.c_str(), value.size());
		uniformTextureBindings.emplace_back(IShader::BindTexture());
	} else if (type == "float") {
		var.SetValue((float)atof(value.c_str()));
	} else if (type == "float2") {
		Float2 v(0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f", &v.x(), &v.y());
		var.SetValue(v);
	} else if (type == "float3") {
		Float3 v(0.0f, 0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f%f", &v.x(), &v.y(), &v.z());
		var.SetValue(v);
	} else if (type == "float4") {
		Float4 v(0.0f, 0.0f, 0.0f, 0.0f);
		sscanf(value.c_str(), "%f%f%f%f", &v.x(), &v.y(), &v.z(), &v.w());
		var.SetValue(v);
	} else if (type == "bool") {
		var.SetValue(value != "false");
	} else if (type == "int") {
		var.SetValue((uint32_t)atoi(value.c_str())); // Hack: encode hash size
	}

	var.key.Assign((const uint8_t*)name.c_str(), safe_cast<uint32_t>(name.size()));

	if (category == "Vertex" || category == "Instance") {
		IShader::BindBuffer bindBuffer;
		var.var = VAR_VERTEX;
		bindBuffer.description.usage = category == "Vertex" ? IRender::Resource::BufferDescription::VERTEX : IRender::Resource::BufferDescription::INSTANCED;
		vertexBufferBindings.emplace_back(std::move(bindBuffer));
	} else if (category == "Input") {
		var.var = VAR_INPUT;
	} else if (category == "Output") {
		var.var = VAR_OUTPUT;
	} else if (category == "Option") {
		var.var = VAR_OPTION;
	} else if (category == "uniform") {
		var.var = VAR_UNIFORM;
	} else {
		assert(false);
	}

	var.schema = 0xFF;
	var.slot = 0xFFFF;
	if (!binding.empty()) {
		var.schema = SchemaFromPredefines(binding, category == "Output");
		if (var.schema == 0xFF) {
			for (size_t i = 0; i < entries.size(); i++) {
				Bytes& key = entries[i].key;
				if (memcmp(binding.c_str(), key.GetData(), Math::Min(binding.size(), (size_t)key.GetSize())) == 0) {
					var.slot = safe_cast<uint16_t>(i);
					break;
				}
			}
		}
	}

	entries.emplace_back(std::move(var));
}

