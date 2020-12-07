#include "GLSLShaderGenerator.h"
#include <sstream>
using namespace PaintsNow;

// Shader Compilation
static const String frameCode = "#version 330\n\
#define PI 3.1415926 \n\
#define GAMMA 2.2 \n\
#define clip(f) if (f < 0) discard; \n\
#define uint2 uvec2 \n\
#define uint3 uvec3 \n\
#define uint4 uvec4 \n\
#define float2 vec2 \n\
#define float3 vec3 \n\
#define float4 vec4 \n\
#define float3x3 mat3 \n\
#define float4x4 mat4 \n\
#define make_float3x3(a11, a12, a13, a21, a22, a23, a31, a32, a33) float3x3(a11, a12, a13, a21, a22, a23, a31, a32, a33) \n\
#define make_float4x4(a11, a12, a13, a14, a21, a22, a23, a24, a31, a32, a33, a34, a41, a42, a43, a44) float4x4(a11, a12, a13, a14, a21, a22, a23, a24, a31, a32, a33, a34, a41, a42, a43, a44) \n\
#define lerp(a, b, v) mix(a, b, v) \n\
#define ddx dFdx \n\
#define ddy dFdy \n\
#define textureShadow texture \n\
float saturate(float x) { return clamp(x, 0.0, 1.0); } \n\
float2 saturate(float2 x) { return clamp(x, float2(0.0, 0.0), float2(1.0, 1.0)); } \n\
float3 saturate(float3 x) { return clamp(x, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0)); } \n\
float4 saturate(float4 x) { return clamp(x, float4(0.0, 0.0, 0.0, 0.0), float4(1.0, 1.0, 1.0, 1.0)); } \n\
#define mult_mat(a, b) (a * b) \n\
#define mult_vec(m, v) (m * v) \n";

// Compute shader frame code
static const String computeFrameCode = "\n\
#define WorkGroupSize gl_WorkGroupSize \n\
#define NumWorkGroups gl_NumWorkGroups \n\
#define LocalInvocationID gl_LocalInvocationID \n\
#define WorkGroupID gl_WorkGroupID \n\
#define GlobalInvocationID gl_GlobalInvocationID \n\
#define LocalInvocationIndex gl_LocalInvocationIndex \n\
\n";

GLSLShaderGenerator::GLSLShaderGenerator(IRender::Resource::ShaderDescription::Stage s, uint32_t& pinputIndex, uint32_t& poutputIndex, uint32_t& ptextureIndex) : IReflect(true, false), stage(s), debugVertexBufferIndex(0), inputIndex(pinputIndex), outputIndex(poutputIndex), textureIndex(ptextureIndex) {}

const String& GLSLShaderGenerator::GetFrameCode() {
	return frameCode;
}

void GLSLShaderGenerator::Complete() {
	for (size_t i = 0; i < bufferBindings.size(); i++) {
		const IShader::BindBuffer* buffer = bufferBindings[i].first;
		const std::pair<String, String>& info = mapBufferDeclaration[buffer];
		switch (buffer->description.usage) {
		case IRender::Resource::BufferDescription::VERTEX:
		case IRender::Resource::BufferDescription::INSTANCED:
			declaration += info.second;
			break;
		case IRender::Resource::BufferDescription::UNIFORM:
			if (!info.second.empty()) {
				declaration += String("uniform _") + info.first + " {\n" + info.second + "} " + info.first + ";\n";
			}
			break;
		case IRender::Resource::BufferDescription::STORAGE:
			if (!info.second.empty()) {
				declaration += String("buffer _") + info.first + " {\n" + info.second + "} " + info.first + ";\n";
			}
			break;
		}
	}
}

static inline String ToString(uint32_t value) {
	std::stringstream ss;
	ss << value;
	return StdToUtf8(ss.str());
}

struct DeclareMap {
	DeclareMap() {
		mapTypeNames[UniqueType<float>::Get()] = "float";
		mapTypeNames[UniqueType<Float2>::Get()] = "float2";
		mapTypeNames[UniqueType<Float3>::Get()] = "float3";
		mapTypeNames[UniqueType<Float4>::Get()] = "float4";
		mapTypeNames[UniqueType<float>::Get()] = "float";
		mapTypeNames[UniqueType<Float2>::Get()] = "float2";
		mapTypeNames[UniqueType<Float3>::Get()] = "float3";
		mapTypeNames[UniqueType<Float4>::Get()] = "float4";
		mapTypeNames[UniqueType<MatrixFloat3x3>::Get()] = "float3x3";
		mapTypeNames[UniqueType<MatrixFloat4x4>::Get()] = "float4x4";
	}

	String operator [] (Unique id) {
		std::map<Unique, String>::iterator it = mapTypeNames.find(id);
		if (it != mapTypeNames.end()) {
			return it->second;
		} else {
			assert(false); // unrecognized
			return "float4";
		}
	}

	std::map<Unique, String> mapTypeNames;
};

void GLSLShaderGenerator::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	// singleton Unique uniqueBindOffset = UniqueType<IShader::BindOffset>::Get();
	singleton Unique uniqueBindInput = UniqueType<IShader::BindInput>::Get();
	singleton Unique uniqueBindOutput = UniqueType<IShader::BindOutput>::Get();
	singleton Unique uniqueBindTexture = UniqueType<IShader::BindTexture>::Get();
	singleton Unique uniqueBindBuffer = UniqueType<IShader::BindBuffer>::Get();
	singleton Unique uniqueBindConstBool = UniqueType<IShader::BindConst<bool> >::Get();
	singleton Unique uniqueBindConstInt = UniqueType<IShader::BindConst<int> >::Get();
	singleton Unique uniqueBindConstFloat = UniqueType<IShader::BindConst<float> >::Get();
	static DeclareMap declareMap;

	if (s.IsBasicObject() || s.IsIterator()) {
		String statement;
		const IShader::BindBuffer* bindBuffer = nullptr;
		String arr;
		String arrDef;
		singleton Unique uniqueBindOption = UniqueType<IShader::BindEnable>::Get();
		singleton Unique uniqueBool = UniqueType<bool>::Get();
		bool isBoolean = typeID == uniqueBool;

		for (const MetaChainBase* pre = meta; pre != nullptr; pre = pre->GetNext()) {
			const MetaNodeBase* node = pre->GetNode();
			Unique uniqueNode = node->GetUnique();
			if (!isBoolean && uniqueNode == uniqueBindOption) {
				const IShader::BindEnable* bindOption = static_cast<const IShader::BindEnable*>(pre->GetNode());
				if (!*bindOption->description) {
					// defines as local
					if (s.IsIterator()) {
						IIterator& iterator = static_cast<IIterator&>(s);
						initialization += String("\t") + declareMap[iterator.GetElementUnique()] + " " + name + "[1];\n";
					} else {
						initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
					}

					return;
				}
			}

			// Bind directly
			if (uniqueNode == uniqueBindBuffer) {
				bindBuffer = static_cast<const IShader::BindBuffer*>(pre->GetRawNode());
			}
		}

		if (s.IsIterator()) {
			IIterator& iterator = static_cast<IIterator&>(s);
			std::stringstream ss;
			ss << "[" << (int)iterator.GetTotalCount() << "]";
			arr = StdToUtf8(ss.str());
			arrDef = "[]";

			typeID = iterator.GetElementUnique();
		}

		for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
			const MetaNodeBase* node = chain->GetNode();
			Unique uniqueNode = node->GetUnique();

			// Bind directly
			if (uniqueNode == uniqueBindInput) {
				const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
				if (bindInput->description == IShader::BindInput::COMPUTE_GROUP) {
					assert(bindBuffer == nullptr);
					// get dimension data
					assert(typeID == UniqueType<UInt3>::Get());
					if (typeID == UniqueType<UInt3>::Get()) {
						const UInt3& data = *reinterpret_cast<UInt3*>(ptr);
						std::stringstream ss;
						ss << "layout (local_size_x = " << data.x() << ", local_size_y = " << data.y() << ", local_size_z = " << data.z() << ") in;\n";
						initialization += StdToUtf8(ss.str());
					}
				} else if (bindInput->description == IShader::BindInput::LOCAL) {
					// Do not declare it here
					// initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
				} else if (bindInput->description == IShader::BindInput::RASTERCOORD) {
					initialization += String("\t") + declareMap[typeID] + " " + name + " = gl_FragCoord;\n";
				} else {
					if (bindBuffer == nullptr || mapBufferEnabled[bindBuffer]) {
						if (stage == IRender::Resource::ShaderDescription::VERTEX) {
							assert(bindBuffer != nullptr);
#ifdef _DEBUG
							if (bindBuffer->description.usage) {
								while (debugVertexBufferIndex < bufferBindings.size()) {
									if (bufferBindings[debugVertexBufferIndex].first == bindBuffer) {
										break;
									}

									debugVertexBufferIndex++;
								}

								// IShader::BindBuffer* must be with the same order as input varyings
								assert(debugVertexBufferIndex < bufferBindings.size());
							}
#endif

							switch (bindBuffer->description.usage) {
							case IRender::Resource::BufferDescription::VERTEX:
								assert(typeID->GetSize() <= 4 * sizeof(float));
								statement += String("layout (location = ") + ToString(inputIndex++) + ") in " + declareMap[typeID] + " " + name + ";\n";
								break;
							case IRender::Resource::BufferDescription::INSTANCED:
								assert(typeID->GetSize() < 4 * sizeof(float) || typeID->GetSize() % (4 * sizeof(float)) == 0);
								statement += String("layout (location = ") + ToString(inputIndex) + ") in " + declareMap[typeID] + " " + name + ";\n";
								inputIndex += ((uint32_t)safe_cast<uint32_t>(typeID->GetSize()) + sizeof(float) * 3) / (sizeof(float) * 4u);
								break;
							}
						} else {
							statement += "in " + declareMap[typeID] + " " + name + ";\n";
							inputIndex++;
						}
					} else {
						// Not enabled, fallback to local
						assert(bindBuffer->description.usage != IRender::Resource::BufferDescription::UNIFORM);
						initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
					}
				}
			} else if (uniqueNode == uniqueBindOutput) {
				const IShader::BindOutput* bindOutput = static_cast<const IShader::BindOutput*>(node);
				if (bindOutput->description == IShader::BindOutput::LOCAL) {
					initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
				} else if (bindOutput->description == IShader::BindOutput::HPOSITION) {
					assert(arr.empty());
					initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
					finalization += String("\tgl_Position = ") + name + ";\n";
				} else {
					if (stage == IRender::Resource::ShaderDescription::FRAGMENT) {
						statement += String("layout (location = ") + ToString(outputIndex++) + ") out " + declareMap[typeID] + " " + name + ";\n";
					} else {
						outputIndex++;
						statement += "out " + declareMap[typeID] + " " + name + ";\n";
					}
				}
			} else if (uniqueNode == uniqueBindConstBool) {
				const IShader::BindConst<bool>* bindOption = static_cast<const IShader::BindConst<bool>*>(node);
				declaration += String("#define ") + name + " " + (bindOption->description ? "true" : "false") + "\n";
			} else if (uniqueNode == uniqueBindConstInt) {
				const IShader::BindConst<int>* bindOption = static_cast<const IShader::BindConst<int>*>(node);
				std::stringstream ss;
				ss << bindOption->description;
				declaration += String("#define ") + name + " " + StdToUtf8(ss.str()) + "\n";
			} else if (uniqueNode == uniqueBindConstFloat) {
				const IShader::BindConst<float>* bindOption = static_cast<const IShader::BindConst<float>*>(node);
				std::stringstream ss;
				ss << bindOption->description;
				declaration += String("#define ") + name + " " + StdToUtf8(ss.str()) + "\n";
			}
		}

		if (bindBuffer != nullptr && mapBufferEnabled[bindBuffer]) {
			std::map<const IShader::BindBuffer*, std::pair<String, String> >::iterator it = mapBufferDeclaration.find(bindBuffer);
			assert(it != mapBufferDeclaration.end());
			if (it != mapBufferDeclaration.end()) {
				switch (bindBuffer->description.usage) {
				case IRender::Resource::BufferDescription::VERTEX:
				case IRender::Resource::BufferDescription::INSTANCED:
					assert(!statement.empty());
					it->second.second += statement;
					break;
				case IRender::Resource::BufferDescription::UNIFORM:
					it->second.second += String("\t") + declareMap[typeID] + " _" + name + arr + ";\n";
					if (stage != IRender::Resource::ShaderDescription::COMPUTE) {
						// initialization += String("\t") + declareMap[typeID] + arrDef + " " + name + " = " + it->second.first + "." + name + ";\n";
						initialization += String("#define ") + name + " " + it->second.first + "." + "_" + name + "\n";
					}
					break;
				}
			} else {
				// these buffer usage do not support structure-mapped layout.
				assert(bindBuffer->description.usage != IRender::Resource::BufferDescription::VERTEX);
				assert(bindBuffer->description.usage != IRender::Resource::BufferDescription::INSTANCED);
				declaration += statement;
			}
		} else {
			declaration += statement;
		}
	} else {
		bool enabled = true;
		for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
			const MetaNodeBase* node = check->GetNode();
			if (node->GetUnique() == UniqueType<IShader::BindEnable>::Get()) {
				const IShader::BindEnable* bind = static_cast<const IShader::BindEnable*>(node);
				if (!*bind->description) {
					enabled = false;
				}
			}
		}

		if (typeID == uniqueBindBuffer) {
			const IShader::BindBuffer* bindBuffer = static_cast<const IShader::BindBuffer*>(&s);
			if (bindBuffer->description.usage != IRender::Resource::BufferDescription::UNIFORM || enabled) {
				mapBufferDeclaration[bindBuffer] = std::make_pair(String(name), String(""));
				mapBufferEnabled[bindBuffer] = enabled;
				if (bindBuffer->description.usage == IRender::Resource::BufferDescription::UNIFORM || IRender::Resource::BufferDescription::STORAGE) {
					bufferBindings.emplace_back(std::make_pair(bindBuffer, String("_") + name));
				} else {
					bufferBindings.emplace_back(std::make_pair(bindBuffer, name));
				}
			}
		} else if (typeID == uniqueBindTexture) {
			assert(enabled);
			const IShader::BindTexture* bindTexture = static_cast<const IShader::BindTexture*>(&s);
			static const char* samplerTypes[] = {
				"sampler1D", "sampler2D", "samplerCube", "sampler3D"
			};

			// declaration += String("layout(location = ") + ToString(textureIndex++) + ") uniform " + samplerTypes[bindTexture->description.state.type] + (bindTexture->description.dimension.z() != 0 && bindTexture->description.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? "Array " : " ") + name + ";\n";
			declaration += String("uniform ") + samplerTypes[bindTexture->description.state.type] + (bindTexture->description.dimension.z() != 0 && bindTexture->description.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? "Array" : "") + (bindTexture->description.state.pcf ? "Shadow " : " ") + name + ";\n";
			textureBindings.emplace_back(std::make_pair(bindTexture, name));
			textureIndex++;
		}
	}
}

void GLSLShaderGenerator::Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}
