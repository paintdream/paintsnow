#include "PassBase.h"
#include <string>

using namespace PaintsNow;

PassBase::PassBase() {}

TObject<IReflect>& PassBase::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

class ReflectCollectShader : public IReflect {
public:
	ReflectCollectShader() : IReflect(true, false) {}
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		static Unique shaderTypeUnique = UniqueType<IShader::MetaShader>::Get();
		while (meta != nullptr) {
			const MetaNodeBase* metaNode = meta->GetNode();
			if (metaNode->GetUnique() == shaderTypeUnique) {
				const IShader::MetaShader* metaShader = static_cast<const IShader::MetaShader*>(metaNode);
				assert(!s.IsBasicObject() && s.QueryInterface(UniqueType<IShader>()) != nullptr);
				shaders.emplace_back(std::make_pair(metaShader->shaderType, static_cast<IShader*>(&s)));
				return;
			}

			meta = meta->GetNext();
		}

		assert(s.IsBasicObject() || s.QueryInterface(UniqueType<IShader>()) == nullptr);
	}

	void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	typedef std::vector<std::pair<IRender::Resource::ShaderDescription::Stage, IShader*> > ShaderMap;
	ShaderMap shaders;
};

IRender::Resource* PassBase::Compile(IRender& render, IRender::Queue* queue) {
	IRender::Resource* shader = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_SHADER);

	if (shader == nullptr) {
		return nullptr;
	}

	IRender::Resource::ShaderDescription shaderDescription;
	ReflectCollectShader allShaders;
	(*this)(allShaders);

	// concat shader text
	shaderDescription.entries = std::move(allShaders.shaders);
	shaderDescription.name = ToString();

	// commit
	render.UploadResource(queue, shader, &shaderDescription);

	return shader;
}

PassBase::Updater::Updater(PassBase& Pass) : textureCount(0), IReflect(true, false, false, false) {
	Pass(*this);
	Flush();
}

uint32_t PassBase::Updater::GetBufferCount() const {
	return safe_cast<uint32_t>(buffers.size());
}

uint32_t PassBase::Updater::GetTextureCount() const {
	return textureCount;
}

PassBase::Parameter::Parameter() : internalAddress(nullptr), linearLayout(0) {}

PassBase::Parameter& PassBase::Updater::operator [] (const Bytes& key) {
	static Parameter defOutput;
	Name input;
	input.schema = IShader::BindInput::GENERAL;
	input.key = key;
	std::map<Name, size_t>::iterator it = mapParameters.find(input);
	return it != mapParameters.end() ? parameters[it->second] : defOutput;
}

PassBase::Parameter& PassBase::Updater::operator [] (IShader::BindInput::SCHEMA schema) {
	static Parameter defOutput;
	Name input;
	input.schema = schema;
	std::map<Name, size_t>::iterator it = mapParameters.find(input);
	return it != mapParameters.end() ? parameters[it->second] : defOutput;
}
	
void PassBase::Updater::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	Name input;
	Bytes byteName;
	byteName.Assign((const uint8_t*)name, safe_cast<uint32_t>(strlen(name)));
	input.key = byteName;
	input.schema = IShader::BindInput::GENERAL;

	Parameter output;
	output.resourceType = IRender::Resource::RESOURCE_UNKNOWN;
	output.type = typeID;
	output.stride = 0;
	output.length = sizeof(IRender::Resource*);
	output.internalAddress = ptr;
	UInt2 subRange(0, ~(uint32_t)0);

	if (s.IsBasicObject() || s.IsIterator()) {
		for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
			const MetaNodeBase* node = chain->GetNode();
			if (node->GetUnique() == UniqueType<IShader::BindOption>::Get()) {
				const IShader::BindOption* bindOption = static_cast<const IShader::BindOption*>(node);
				if (!*bindOption->description) break;
			} else if (node->GetUnique() == UniqueType<IShader::BindInput>::Get()) {
				const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
				input.schema = safe_cast<IShader::BindInput::SCHEMA>(bindInput->description);
				if (bindInput->subRangeQueryer) {
					assert(output.resourceType == IRender::Resource::RESOURCE_UNKNOWN); // BindInput must executed before BindBuffer in this case. i.e. place BindInput AFTER BindBuffer in your reflection handler.
					subRange = bindInput->subRangeQueryer();
				}
			} else if (node->GetUnique() == UniqueType<IShader::BindBuffer>::Get()) {
				const IShader::BindBuffer* bindBuffer = static_cast<const IShader::BindBuffer*>(chain->GetRawNode());
				output.resourceType = safe_cast<uint8_t>(IRender::Resource::RESOURCE_BUFFER);
				if (bufferID.find(bindBuffer) != bufferID.end()) {
					output.slot = safe_cast<uint8_t>(bufferID[bindBuffer]);
					output.offset = safe_cast<uint16_t>(bufferSize[bindBuffer]);
					uint32_t size;
					if (s.IsIterator()) {
						IIterator& iterator = static_cast<IIterator&>(s);
						assert(iterator.IsLayoutLinear());
						uint32_t count = safe_cast<uint32_t>(iterator.GetTotalCount());
						assert(count != 0);
						size = (uint32_t)safe_cast<uint32_t>(iterator.GetPrototypeUnique()->GetSize()) * count;

						iterator.Next();
						output.internalAddress = iterator.Get();

						if (subRange.y() != ~(uint32_t)0) {
							output.internalAddress = (uint8_t*)output.internalAddress + subRange.x();
							assert(subRange.x() + subRange.y() <= size);
							size = subRange.y();
						}
					} else {
						size = safe_cast<uint32_t>(typeID->GetSize());
					}

					output.length = size;
					bufferSize[bindBuffer] += size;
				} else {
#ifdef _DEBUG
					assert(disabled.count(bindBuffer) != 0);
#endif
					return;
				}
			}

			// Texture binding port is determined by BindTexture property itself
			/*else if (node->GetUnique() == UniqueType<IShader::BindTexture>::Get()) {
				const IShader::BindTexture* bindTexture = static_cast<const IShader::BindTexture*>(chain->GetRawNode());
				output.resourceType = (uint8_t)IRender::Resource::RESOURCE_TEXTURE;
				if (textureID.find(bindTexture) != textureID.end()) {
					output.slot = (uint8_t)textureID[bindTexture];
					output.offset = 0;
				} else {
#ifdef _DEBUG
						assert(disabled.count(bindTexture) != 0);
#endif
						return;
					}
				}*/
		}

		if (output.resourceType != IRender::Resource::RESOURCE_UNKNOWN) {
			mapParameters[input] = parameters.size();
			parameters.emplace_back(output);
		}
	} else {
		// check if disabled
		for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
			const MetaNodeBase* node = check->GetNode();
			if (node->GetUnique() == UniqueType<IShader::MetaShader>::Get()) {
				const IShader::MetaShader* metaShader = static_cast<const IShader::MetaShader*>(node);
				s(*this);
			} else if (node->GetUnique() == UniqueType<IShader::BindOption>::Get()) {
				const IShader::BindOption* bind = static_cast<const IShader::BindOption*>(node);
				if (!*bind->description) {
					// disabled skip.
#ifdef _DEBUG
					disabled.insert(&s);
#endif
					return;
				}
			}
		}

		if (typeID == UniqueType<IShader::BindBuffer>::Get()) {
			IShader::BindBuffer* buffer = static_cast<IShader::BindBuffer*>(&s);
			uint32_t id = safe_cast<uint32_t>(bufferID.size());
			bufferID[buffer] = id;
			bufferSize[buffer] = 0;
			buffers.emplace_back(buffer);
		} else if (typeID == UniqueType<IShader::BindTexture>::Get()) {
			IShader::BindTexture* texture = static_cast<IShader::BindTexture*>(&s);
			output.resourceType = safe_cast<uint8_t>(IRender::Resource::RESOURCE_TEXTURE);
			output.slot = safe_cast<uint8_t>(textureCount++);
			output.offset = 0;
			output.internalAddress = &texture->resource;
			output.type = UniqueType<IRender::Resource*>::Get();

			mapParameters[input] = parameters.size();
			parameters.emplace_back(output);
		}
	}
}

void PassBase::Updater::Flush() {
	std::vector<uint8_t> fixBufferSlots(buffers.size());

	std::vector<const IShader::BindBuffer*> fixedBuffers;
	for (size_t i = 0; i < buffers.size(); i++) {
		// Empty buffer?
		fixBufferSlots[i] = safe_cast<uint32_t>(fixedBuffers.size());
		if (bufferSize[buffers[i]] != 0) {
			fixedBuffers.push_back(buffers[i]);
		}
	}

	// generate quick updaters
	quickUpdaters.reserve(parameters.size());

	// compute buffer linearness
	std::vector<bool> bufferLinearness(buffers.size(), true);
	std::vector<const char*> bufferAddresses(buffers.size(), nullptr);

	for (size_t k = 0; k < parameters.size(); k++) {
		Parameter& param = parameters[k];
		if (param.resourceType == IRender::Resource::RESOURCE_BUFFER) {
			param.stride = safe_cast<uint16_t>(bufferSize[buffers[param.slot]]);
			const char* baseAddress = (const char*)param.internalAddress - param.offset;
			if (bufferAddresses[param.slot] == nullptr) {
				bufferAddresses[param.slot] = baseAddress;
			} else if (bufferLinearness[param.slot]) {
				bufferLinearness[param.slot] = bufferAddresses[param.slot] == baseAddress;
			}
		}
	}

	for (size_t j = 0; j < parameters.size(); j++) {
		Parameter& param = parameters[j];
		if (param.resourceType == IRender::Resource::RESOURCE_BUFFER) {
			if (bufferLinearness[param.slot]) {
				if (param.offset != 0)
					continue;

				param.linearLayout = 1;
			}
		
			param.slot = fixBufferSlots[param.slot]; // fix slot
		}

		quickUpdaters.emplace_back(j);
	}

	std::swap(buffers, fixedBuffers);
	bufferSize.clear();
	bufferID.clear();
}

void PassBase::Updater::Capture(IRender::Resource::DrawCallDescription& drawCallDescription, std::vector<Bytes>& bufferData, uint32_t bufferMask) {
	drawCallDescription.bufferResources.resize(buffers.size());

	for (size_t k = 0; k < quickUpdaters.size(); k++) {
		const Parameter& parameter = parameters[quickUpdaters[k]];
		if (parameter.resourceType == IRender::Resource::RESOURCE_TEXTURE) {
			if (drawCallDescription.textureResources.size() <= parameter.slot) {
				drawCallDescription.textureResources.resize(parameter.slot + 1);
			}

			drawCallDescription.textureResources[parameter.slot] = *reinterpret_cast<IRender::Resource**>(parameter.internalAddress);
		} else if (parameter.resourceType == IRender::Resource::RESOURCE_BUFFER) {
			const IShader::BindBuffer* bindBuffer = buffers[parameter.slot];
			if ((bufferMask & (1 << bindBuffer->description.usage)) && bindBuffer->resource == nullptr) {
				if (bufferData.size() <= parameter.slot) {
					bufferData.resize(parameter.slot + 1);
				}

				Bytes& s = bufferData[parameter.slot];
				if (s.Empty()) s.Resize(parameter.stride);
				memcpy(s.GetData() + parameter.offset, parameter.internalAddress, parameter.linearLayout ? parameter.stride : parameter.length);
			}
		}
	}
}

void PassBase::Updater::Update(IRender& render, IRender::Queue* queue, IRender::Resource::DrawCallDescription& drawCallDescription, std::vector<IRender::Resource*>& newBuffers, std::vector<Bytes>& bufferData, uint32_t bufferMask) {
	for (size_t i = 0; i < buffers.size(); i++) {
		const IShader::BindBuffer* bindBuffer = buffers[i];
		if ((bufferMask & (1 << bindBuffer->description.usage))) {
			assert(i < drawCallDescription.bufferResources.size());
			IRender::Resource*& buffer = drawCallDescription.bufferResources[i].buffer;
			if (bindBuffer->resource != nullptr) {
				buffer = bindBuffer->resource;
			} else {
				if (buffer == nullptr) {
					buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
					newBuffers.emplace_back(buffer);
				}

				IRender::Resource::BufferDescription desc = bindBuffer->description;
				desc.data = std::move(bufferData[i]);
				assert(desc.data.GetSize() != 0);
				render.UploadResource(queue, buffer, &desc);
			}
		}
	}
}

template <bool constantOnly>
class HashExporter : public IReflect {
public:
	Bytes hashValue;
	HashExporter() : IReflect(true, false, false, false) {}
	
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (s.IsBasicObject()) {
			for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
				const MetaNodeBase* node = chain->GetNode();
				Unique unique = node->GetUnique();
				if (unique == UniqueType<IShader::BindConst<bool> >::Get()
				|| unique == UniqueType<IShader::BindConst<uint32_t> >::Get()
				|| unique == UniqueType<IShader::BindConst<uint16_t> >::Get()
				|| unique == UniqueType<IShader::BindConst<uint8_t> >::Get()
				|| (!constantOnly && unique == UniqueType<IShader::BindInput>::Get())) {
					hashValue.Append(reinterpret_cast<uint8_t*>(ptr), safe_cast<uint32_t>(typeID->GetSize()));
					break;
				}
			}
		} else {
			for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
				const MetaNodeBase* node = check->GetNode();
				if (node->GetUnique() == UniqueType<IShader::MetaShader>::Get()) {
					const IShader::MetaShader* metaShader = static_cast<const IShader::MetaShader*>(node);
					s(*this);
				}
			}
		}
	}

	void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}
};

Bytes PassBase::ExportHash(bool constantOnly) {
	if (constantOnly) {
		HashExporter<true> exporter;
		(*this)(exporter);
		return exporter.hashValue;
	} else {
		HashExporter<false> exporter;
		(*this)(exporter);
		return exporter.hashValue;
	}
}

bool PassBase::Name::operator < (const Name& rhs) const {
	if (schema == IShader::BindInput::GENERAL && rhs.schema == IShader::BindInput::GENERAL) {
		return key < rhs.key;
	}

	return schema < rhs.schema;
}

PassBase::~PassBase() {}

void PassBase::PartialUpdater::Snapshot(std::vector<Bytes>& bufferData, std::vector<IRender::Resource::DrawCallDescription::BufferRange>& bufferResources, std::vector<IRender::Resource*>& textureResources, const PassBase::PartialData& partialData) const {
	static Unique uniqueBindBuffer = UniqueType<IShader::BindBuffer>::Get();
	static Unique uniqueBindTexture = UniqueType<IShader::BindTexture>::Get();

	std::vector<uint32_t> starts(bufferData.size(), ~(uint32_t)0);

	for (size_t i = 0; i < parameters.size(); i++) {
		const Parameter& parameter = parameters[i];
		if (parameter.type == uniqueBindBuffer) {
			const IShader::BindBuffer* buffer = reinterpret_cast<const IShader::BindBuffer*>((const char*)&partialData + (size_t)parameter.internalAddress);
			if (bufferResources.size() <= parameter.slot) {
				bufferResources.resize(parameter.slot + 1);
			}

			IRender::Resource::DrawCallDescription::BufferRange& range = bufferResources[parameter.slot];
			range.buffer = buffer->resource;
			range.offset = 0;
			range.length = 0;
		} else if (parameter.type == uniqueBindTexture) {
			const IShader::BindTexture* texture = reinterpret_cast<const IShader::BindTexture*>((const char*)&partialData + (size_t)parameter.internalAddress);
			if (textureResources.size() <= parameter.slot) {
				textureResources.resize(parameter.slot + 1);
			}

			textureResources[parameter.slot] = texture->resource;
		} else {
			if (bufferData.size() <= parameter.slot) {
				bufferData.resize(parameter.slot + 1);
				starts.resize(parameter.slot + 1, ~(uint32_t)0);
			}

			assert(parameter.slot < bufferData.size());
			Bytes& buffer = bufferData[parameter.slot];
			const PassBase::Parameter& p = parameters[i];
			uint32_t& start = starts[parameter.slot];
			if (start == ~(uint32_t)0) {
				start = safe_cast<uint32_t>(buffer.GetSize());
				buffer.Resize(start + p.stride);
			}

			memcpy(buffer.GetData() + start + p.offset, (const char*)&partialData + (size_t)p.internalAddress, p.type->GetSize());
		}
	}
}

Bytes PassBase::PartialUpdater::ComputeHash() const {
	if (parameters.empty()) return Bytes::Null();

	Bytes buffer(safe_cast<uint32_t>(parameters.size() * sizeof(uint16_t)));
	uint16_t* p = (uint16_t*)buffer.GetData();
	for (size_t i = 0; i < parameters.size(); i++) {
		const PassBase::Parameter& output = parameters[i];
		static_assert(sizeof(output.offset) == sizeof(uint16_t), "Size check");
		p[i] = output.offset;
	}

	return buffer;
}

class ReflectPartial : public IReflect {
public:
	ReflectPartial(PassBase::Updater& u) : IReflect(true), updater(u) {}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (s.IsBasicObject()) {
			PassBase::Parameter* m = nullptr;
			for (const MetaChainBase* p = meta; p != nullptr; p = p->GetNext()) {
				const MetaNodeBase* node = p->GetNode();
				if (node->GetUnique() == UniqueType<IShader::BindInput>::Get()) {
					const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
					m = &updater[IShader::BindInput::SCHEMA(bindInput->description)];
				}
			}

			// search for same 
			Bytes byteName;
			byteName.Assign((const uint8_t*)name, safe_cast<uint32_t>(strlen(name)));
			PassBase::Parameter& parameter = m != nullptr && *m ? *m : updater[byteName];

			if (parameter) {
				parameter.internalAddress = (void*)((size_t)ptr - (size_t)base);
				outputs.emplace_back(parameter);
			}
		}
	}

	std::vector<PassBase::Parameter> outputs;
	PassBase::Updater& updater;
};

void PassBase::PartialData::Export(PartialUpdater& particalUpdater, PassBase::Updater& updater) const {
	ReflectPartial reflector(updater);
	(*const_cast<PassBase::PartialData*>(this))(reflector);

	if (!reflector.outputs.empty()) {
		uint32_t sum = 0;
		for (size_t i = 0; i < reflector.outputs.size(); i++) {
			Parameter& parameter = reflector.outputs[i];
			sum += (uint32_t)safe_cast<uint32_t>(parameter.type->GetSize());
		}
	
		// check completeness
		assert(sum == reflector.outputs[0].stride); // must provide all segments by now
	}

	particalUpdater.parameters = std::move(reflector.outputs);
}
