#include "CustomMaterialParameterFS.h"
#include <sstream>

using namespace PaintsNow;

CustomMaterialDescription::CustomMaterialDescription() : dependency(nullptr), dependOptBuffer(nullptr) {
	uniformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

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
		return UniqueType<MatrixFloat4x4>::Get();
	case IAsset::TYPE_TEXTURE:
		return UniqueType<IRender::Resource*>::Get();
	default:
		assert(false);
		return UniqueType<float>::Get();
	}
}

static uint8_t SchemaFromPredefines(const String& binding, bool input) {
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
	} else if (binding == "TRANSFORM_WORLD") {
		if (input) {
			return IShader::BindInput::TRANSFORM_WORLD;
		}
	} else if (binding == "TRANSFORM_VIEW") {
		if (input) {
			return IShader::BindInput::TRANSFORM_VIEW;
		}
	} else if (binding == "TRANSFORM_VIEWPROJECTION") {
		if (input) {
			return IShader::BindInput::TRANSFORM_VIEWPROJECTION;
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

void CustomMaterialDescription::ReflectUniformTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	uint32_t offset = 0;
	uint8_t* bufferBase = extUniformBuffer.GetData();

	// Expand
	// ReflectProperty(uniformBuffer);
	singleton Unique typeBuffer = UniqueType<IShader::BindBuffer>::Get();
	std::stringstream ss;
	ss << "uniformBuffer_" << (void*)bufferBase;
	reflect.Property(uniformBuffer, typeBuffer, typeBuffer, StdToUtf8(ss.str()).c_str(), this, &uniformBuffer, nullptr);

	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_UNIFORM) continue;

		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		if (var.type == IAsset::TYPE_TEXTURE) {
			uint32_t index = var.offset;
			reflect.Property(uniformTextureBindings[index], UniqueType<IShader::BindTexture>::Get(), UniqueType<IShader::BindTexture>::Get(), name.c_str(), &uniformTextureBindings[0], &uniformTextureBindings[index], nullptr);
		} else {
			Unique type = UniqueFromEntryType(var.type);
			static IReflectObject dummy;
			DummyMetaChain<IShader::BindBuffer> chain(uniformBuffer);
			// Make alignment
			uint32_t size = safe_cast<uint32_t>(type->GetSize());
			offset = (offset + size - 1) & (size - 1); // make alignment for variable
			if (var.slot != 0xFFFF) { // depends
				assert(entries[var.slot].var == VAR_OPTION);
				IShader::BindEnable enable((bool&)extOptionBuffer[entries[var.slot].offset]);
				DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
				reflect.Property(dummy, type, type, name.c_str(), bufferBase + offset, bufferBase + offset + size, &enableChain);
			} else {
				reflect.Property(dummy, type, type, name.c_str(), bufferBase + offset, bufferBase + offset + size, &chain);
			}

			offset += size;
		}
	}
}

void CustomMaterialDescription::ReflectVertexTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_VERTEX) continue;

		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromEntryType(var.type);
		static IReflectObject dummy;
		uint32_t index = var.offset;
		if (var.slot != 0xFFFF) { // depends
			assert(entries[var.slot].var == VAR_OPTION);
			IShader::BindEnable enable((bool&)extOptionBuffer[entries[var.slot].offset]);
			DummyMetaChain<IShader::BindEnable> enableChain(enable);
			reflect.Property(vertexBufferBindings[index], UniqueType<IShader::BindBuffer>::Get(), UniqueType<IShader::BindBuffer>::Get(), name.c_str(), &vertexBufferBindings[0], &vertexBufferBindings[index], &enableChain);
		} else {
			reflect.Property(vertexBufferBindings[index], UniqueType<IShader::BindBuffer>::Get(), UniqueType<IShader::BindBuffer>::Get(), name.c_str(), &vertexBufferBindings[0], &vertexBufferBindings[index], nullptr);
		}

		IShader::BindInput slot(var.schema);
		DummyMetaChain<IShader::BindInput> chainInput(slot);
		DummyMetaChain<IShader::BindBuffer> chain(vertexBufferBindings[index], &chainInput);

		if (var.slot != 0xFFFF) { // depends
			assert(entries[var.slot].var == VAR_OPTION);
			IShader::BindEnable enable((bool&)extOptionBuffer[entries[var.slot].offset]);
			DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &enableChain); // use var.value.GetData() to provide default value
		} else {
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &chain); // use var.value.GetData() to provide default value
		}
	}
}

void CustomMaterialDescription::ReflectOptionTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
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

		// bool?
		if (size == 1) {
			Unique type = UniqueType<bool>::Get();
			bool& enabled = (bool&)extOptionBuffer[offset];

			if (var.slot != 0xFFFF) {
				Entry& target = entries[var.slot];
				switch (target.var) {
				case VAR_VERTEX:
					enabled = vertexBufferBindings[var.offset];
					break;
				case VAR_UNIFORM:
					if (target.type == IAsset::TYPE_TEXTURE) {
						enabled = uniformTextureBindings[var.offset];
					}
					break;
				case VAR_OPTION:
					enabled = extOptionBuffer[var.offset] != 0;
					break;
				}
			}

			IShader::BindConst<bool> bindConst(enabled);
			DummyMetaChain<IShader::BindConst<bool> > chain(bindConst);
			reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &chain);
		} else { // int
			assert(size == sizeof(uint32_t));
			IShader::BindConst<uint32_t> slot(*(uint32_t*)&extOptionBuffer[offset]);
			DummyMetaChain<IShader::BindConst<uint32_t> > chain(slot);
			Unique type = UniqueType<uint32_t>::Get();
			reflect.Property(dummy, type, type, name.c_str(), bufferBase, bufferBase + offset, &chain);
		}

		offset += size;
	}
}

void CustomMaterialDescription::ReflectInputTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_INPUT) continue;

		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromEntryType(var.type);
		static IReflectObject dummy;
		IShader::BindInput slot(var.schema);
		DummyMetaChain<IShader::BindInput> chain(slot);

		if (var.slot != 0xFFFF) {
			assert(entries[var.slot].var == VAR_OPTION);
			IShader::BindEnable enable((bool&)extOptionBuffer[entries[var.slot].offset]);
			DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &enableChain);
		} else {
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &chain);
		}
	}
}

void CustomMaterialDescription::ReflectOutputTemplate(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	for (size_t i = 0; i < entries.size(); i++) {
		Entry& var = entries[i];
		if (var.var != VAR_OUTPUT) continue;

		String name;
		name.assign((const char*)var.key.GetData(), var.key.GetSize());

		Unique type = UniqueFromEntryType(var.type);
		static IReflectObject dummy;
		IShader::BindOutput slot(var.schema);
		DummyMetaChain<IShader::BindOutput> chain(slot);

		if (var.slot != 0xFFFF) {
			assert(entries[var.slot].var == VAR_OPTION);
			IShader::BindEnable enable((bool&)extOptionBuffer[entries[var.slot].offset]);
			DummyMetaChain<IShader::BindEnable> enableChain(enable, &chain);
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &enableChain);
		} else {
			reflect.Property(dummy, type, type, name.c_str(), nullptr, var.value.GetData(), &chain);
		}
	}
}

void CustomMaterialDescription::ReflectExternal(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer) {
	if (reflect.IsReflectProperty()) {
		if (dependency != nullptr) {
			assert(dependOptBuffer != nullptr);
			SyncOptions(extOptionBuffer, *dependency, *dependOptBuffer);
		}

		ReflectUniformTemplate(reflect, extUniformBuffer, extOptionBuffer);
		ReflectOptionTemplate(reflect, extUniformBuffer, extOptionBuffer);
		ReflectVertexTemplate(reflect, extUniformBuffer, extOptionBuffer);
		ReflectInputTemplate(reflect, extUniformBuffer, extOptionBuffer);
		ReflectOutputTemplate(reflect, extUniformBuffer, extOptionBuffer);
	}
}

void CustomMaterialDescription::SetComplete(Bytes& uniformBufferData, Bytes& optionBufferData) {
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
			if (var.value.GetSize() == sizeof(bool)) {
				optionBufferData.Append(var.value.GetData(), sizeof(bool));
				optionOffset++;
			} else {
				assert(var.value.GetSize() == sizeof(uint32_t));
				// padding
				const size_t size = sizeof(uint32_t);
				optionOffset = (optionOffset + size - 1) & (size - 1);

				uint32_t value = *reinterpret_cast<uint32_t*>(var.value.GetData());
				optionBufferData.Resize(optionOffset + sizeof(value), false);
				*(uint32_t*)(optionBufferData.GetData() + optionOffset) = value;

				var.offset = safe_cast<uint32_t>(optionOffset);
				optionOffset += sizeof(value);
			}
		}
	}
}

void CustomMaterialDescription::SetCode(const String& text) {
	code = text;
}

void CustomMaterialDescription::SetInput(const String& category, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {
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
	} else if (type == "float3x3") {
		MatrixFloat3x3 mat = MatrixFloat3x3::Identity();
		sscanf(value.c_str(), "%f%f%f%f%f%f%f%f%f",
			&mat(0, 0), &mat(0, 1), &mat(0, 2),
			&mat(1, 0), &mat(1, 1), &mat(1, 2),
			&mat(2, 0), &mat(2, 1), &mat(2, 2));
		var.SetValue(mat);
	} else if (type == "float4x4") {
		MatrixFloat4x4 mat = MatrixFloat4x4::Identity();
		sscanf(value.c_str(), "%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f",
			&mat(0, 0), &mat(0, 1), &mat(0, 2), &mat(0, 3),
			&mat(1, 0), &mat(1, 1), &mat(1, 2), &mat(1, 3),
			&mat(2, 0), &mat(2, 1), &mat(2, 2), &mat(2, 3),
			&mat(3, 0), &mat(3, 1), &mat(3, 2), &mat(3, 3));
		var.SetValue(mat);
	} else if (type == "bool") {
		var.SetValue(value != "false");
	} else if (type == "int") {
		var.SetValue((uint32_t)atoi(value.c_str()));
	} else {
		assert(false);
	}

	var.key.Assign((const uint8_t*)name.c_str(), safe_cast<uint32_t>(name.size()));

	if (category == "Vertex" || category == "Instance") {
		IShader::BindBuffer bindBuffer;
		var.var = VAR_VERTEX;
		bindBuffer.description.usage = category == "Vertex" ? IRender::Resource::BufferDescription::VERTEX : IRender::Resource::BufferDescription::INSTANCED;
		var.offset = safe_cast<uint16_t>(vertexBufferBindings.size());
		vertexBufferBindings.emplace_back(std::move(bindBuffer));
	} else if (category == "Input") {
		var.var = VAR_INPUT;
	} else if (category == "Output") {
		var.var = VAR_OUTPUT;
	} else if (category == "Option") {
		var.var = VAR_OPTION;
	} else if (category == "Uniform") {
		var.var = VAR_UNIFORM;
	} else {
		assert(false);
	}

	var.schema = 0xFF;
	var.slot = 0xFFFF;
	if (!binding.empty()) {
		var.schema = SchemaFromPredefines(binding, category != "Input");
		if (var.schema == 0xFF) {
			for (size_t i = 0; i < entries.size(); i++) {
				Entry& entry = entries[i];
				Bytes& key = entry.key;
				if (memcmp(binding.c_str(), key.GetData(), Math::Min(binding.size(), (size_t)key.GetSize())) == 0) {
					assert(var.var == VAR_OPTION || entry.var == VAR_OPTION);
					var.slot = safe_cast<uint16_t>(i);
					break;
				}
			}

			// assert(var.slot != 0xFFFF);
		}
	}

	entries.emplace_back(std::move(var));
}

void CustomMaterialDescription::SyncOptions(Bytes& dstOptionBuffer, const CustomMaterialDescription& rhs, const Bytes& srcOptionBuffer) {
	for (size_t i = 0; i < entries.size(); i++) {
		Entry& target = entries[i];

		if (target.var == VAR_OPTION && target.value.GetSize() == 1) {
			for (size_t j = 0; j < entries.size(); j++) {
				const Entry& source = rhs.entries[j];

				if (source.var == VAR_OPTION && source.value.GetSize() == 1) {
					if (target.key == source.key) {
						dstOptionBuffer[target.offset] = srcOptionBuffer[source.offset];
						break;
					}
				}
			}
		}
	}
}

