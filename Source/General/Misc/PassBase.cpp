#include "PassBase.h"
#include <string>

using namespace PaintsNow;

PassBase::PassBase() {}

void PassBase::SetInput(const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {}
void PassBase::SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) {}
void PassBase::SetComplete() {}

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

IRender::Resource* PassBase::Compile(IRender& render, IRender::Queue* queue, const TWrapper<void, IRender::Resource*, IRender::Resource::ShaderDescription&, IRender::Resource::ShaderDescription::Stage, const String&, const String&>& callback, void* context, IRender::Resource* existedShaderResource) {
	IRender::Resource* shader = existedShaderResource != nullptr ? existedShaderResource : render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_SHADER);

	IRender::Resource::ShaderDescription shaderDescription;
	ReflectCollectShader allShaders;
	(*this)(allShaders);

	// concat shader text
	shaderDescription.entries = std::move(allShaders.shaders);
	shaderDescription.compileCallback = callback;
	shaderDescription.context = context;
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

PassBase::Parameter::Parameter() : internalAddress(nullptr), linearLayout(0), bindBuffer(nullptr) {}

const PassBase::Parameter& PassBase::Updater::operator [] (const Bytes& key) {
	static Parameter defOutput;
	std::vector<std::key_value<Bytes, uint32_t> >::iterator it = std::binary_find(mapParametersKey.begin(), mapParametersKey.end(), key);
	return it != mapParametersKey.end() ? parameters[it->second] : defOutput;
}

const PassBase::Parameter& PassBase::Updater::operator [] (IShader::BindInput::SCHEMA schema) {
	static Parameter defOutput;
	std::vector<std::key_value<uint32_t, uint32_t> >::iterator it = std::binary_find(mapParametersSchema.begin(), mapParametersSchema.end(), schema);
	return it != mapParametersSchema.end() ? parameters[it->second] : defOutput;
}
	
void PassBase::Updater::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	Bytes byteName;
	byteName.Assign((const uint8_t*)name, safe_cast<uint32_t>(strlen(name)));
	uint32_t schema = IShader::BindInput::GENERAL;

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
			if (node->GetUnique() == UniqueType<IShader::BindEnable>::Get()) {
				const IShader::BindEnable* bindOption = static_cast<const IShader::BindEnable*>(node);
				if (!*bindOption->description) break;
			} else if (node->GetUnique() == UniqueType<IShader::BindInput>::Get()) {
				const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
				schema = safe_cast<IShader::BindInput::SCHEMA>(bindInput->description);
				if (bindInput->subRangeQueryer) {
					assert(output.resourceType == IRender::Resource::RESOURCE_UNKNOWN); // BindInput must executed before BindBuffer in this case. i.e. place BindInput AFTER BindBuffer in your reflection handler.
					subRange = bindInput->subRangeQueryer();
				}
			} else if (node->GetUnique() == UniqueType<IShader::BindBuffer>::Get()) {
				const IShader::BindBuffer* bindBuffer = static_cast<const IShader::BindBuffer*>(chain->GetRawNode());
				output.resourceType = safe_cast<uint8_t>(IRender::Resource::RESOURCE_BUFFER);
				output.bindBuffer = const_cast<IShader::BindBuffer*>(bindBuffer);

				std::vector<std::key_value<const IShader::BindBuffer*, std::pair<uint16_t, uint16_t> > >::iterator it = std::binary_find(bufferIDSize.begin(), bufferIDSize.end(), bindBuffer);

				if (it != bufferIDSize.end()) {
					output.slot = safe_cast<uint8_t>(it->second.first);
					output.offset = safe_cast<uint16_t>(it->second.second);
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
					it->second.second += size;
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
			std::binary_insert(mapParametersSchema, std::make_key_value(schema, (uint32_t)safe_cast<uint32_t>(parameters.size())));
			std::binary_insert(mapParametersKey, std::make_key_value(byteName, (uint32_t)safe_cast<uint32_t>(parameters.size())));
			parameters.emplace_back(output);
		}
	} else {
		// check if disabled
		for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
			const MetaNodeBase* node = check->GetNode();
			if (node->GetUnique() == UniqueType<IShader::MetaShader>::Get()) {
				const IShader::MetaShader* metaShader = static_cast<const IShader::MetaShader*>(node);
				s(*this);
			} else if (node->GetUnique() == UniqueType<IShader::BindInput>::Get()) {
				const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
				schema = safe_cast<IShader::BindInput::SCHEMA>(bindInput->description);
			} else if (node->GetUnique() == UniqueType<IShader::BindEnable>::Get()) {
				const IShader::BindEnable* bind = static_cast<const IShader::BindEnable*>(node);
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
			const IShader::BindBuffer* buffer = static_cast<IShader::BindBuffer*>(&s);
			uint32_t id = safe_cast<uint32_t>(bufferIDSize.size());
			std::binary_insert(bufferIDSize, std::make_key_value(buffer, std::make_pair((uint16_t)id, (uint16_t)0)));
			buffers.emplace_back(buffer);
		} else if (typeID == UniqueType<IShader::BindTexture>::Get()) {
			IShader::BindTexture* texture = static_cast<IShader::BindTexture*>(&s);
			output.resourceType = safe_cast<uint8_t>(IRender::Resource::RESOURCE_TEXTURE);
			output.slot = safe_cast<uint8_t>(textureCount++);
			output.offset = 0;
			output.internalAddress = &texture->resource;
			output.type = UniqueType<IRender::Resource*>::Get();

			std::binary_insert(mapParametersSchema, std::make_key_value(schema, (uint32_t)safe_cast<uint32_t>(parameters.size())));
			std::binary_insert(mapParametersKey, std::make_key_value(byteName, (uint32_t)safe_cast<uint32_t>(parameters.size())));
			parameters.emplace_back(output);
		}
	}
}

void PassBase::Updater::Flush() {
	std::vector<uint8_t> fixBufferSlots(buffers.size());

	std::vector<const IShader::BindBuffer*> fixedBuffers;
	for (uint32_t i = 0; i < buffers.size(); i++) {
		// Empty buffer?
		fixBufferSlots[i] = safe_cast<uint32_t>(fixedBuffers.size());
		std::vector<std::key_value<const IShader::BindBuffer*, std::pair<uint16_t, uint16_t> > >::iterator it = std::binary_find(bufferIDSize.begin(), bufferIDSize.end(), buffers[i]);
		if (it->second.second != 0) {
			fixedBuffers.push_back(buffers[i]);
		}
	}

	// generate quick updaters
	quickUpdaters.reserve(parameters.size());

	// compute buffer linearness
	std::vector<bool> bufferLinearness(buffers.size(), true);
	std::vector<const char*> bufferAddresses(buffers.size(), nullptr);

	for (uint32_t k = 0; k < parameters.size(); k++) {
		Parameter& param = parameters[k];
		if (param.resourceType == IRender::Resource::RESOURCE_BUFFER) {
			std::vector<std::key_value<const IShader::BindBuffer*, std::pair<uint16_t, uint16_t> > >::iterator it = std::binary_find(bufferIDSize.begin(), bufferIDSize.end(), buffers[param.slot]);
			param.stride = it->second.second;
			const char* baseAddress = (const char*)param.internalAddress - param.offset;
			if (bufferAddresses[param.slot] == nullptr) {
				bufferAddresses[param.slot] = baseAddress;
			} else if (bufferLinearness[param.slot]) {
				bufferLinearness[param.slot] = bufferAddresses[param.slot] == baseAddress;
			}
		}
	}

	for (uint32_t j = 0; j < parameters.size(); j++) {
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
	bufferIDSize.clear();
}

void PassBase::Updater::Capture(IRender::Resource::DrawCallDescription& drawCallDescription, std::vector<Bytes>& bufferData, uint32_t bufferMask) {
	drawCallDescription.bufferResources.resize(buffers.size());

	for (uint32_t k = 0; k < quickUpdaters.size(); k++) {
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
	for (uint32_t i = 0; i < buffers.size(); i++) {
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
				|| unique == UniqueType<IShader::BindConst<uint8_t> >::Get()) {
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


Bytes PassBase::ExportHash() const {
	HashExporter exporter;
	(const_cast<PassBase&>(*this))(exporter);

	return exporter.hashValue;
}

class SwitchEvaluater : public IReflect {
public:
	SwitchEvaluater(uint32_t mask) : IReflect(true, false, false, false), resourceMask(mask), next(false) {}

	uint32_t resourceMask;
	bool next;

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (s.IsBasicObject()) {
			if (typeID == UniqueType<bool>::Get()) {
				bool& value = *reinterpret_cast<bool*>(ptr);

				for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
					const MetaNodeBase* node = chain->GetNode();
					Unique unique = node->GetUnique();

					if (unique == UniqueType<IShader::BindConst<bool> >::Get()) {
						const IShader::BindConst<bool>* bindConst = static_cast<const IShader::BindConst<bool>*>(node);
						if (value != bindConst->description) {
							value = bindConst->description;
							next = true;
						}
					}
				}
			}
		}
	}

	void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}
};

bool PassBase::FlushSwitches(uint32_t resourceMask) {
	int i = 0;
	while (true) {
		SwitchEvaluater evaluator(resourceMask);
		(const_cast<PassBase&>(*this))(evaluator);

		if (!evaluator.next) {
			return i != 0;
		}

		i++;
	}

	return true;
}

PassBase::~PassBase() {}

void PassBase::PartialUpdater::Snapshot(std::vector<Bytes>& bufferData, std::vector<IRender::Resource::DrawCallDescription::BufferRange>& bufferResources, std::vector<IRender::Resource*>& textureResources, const PassBase::PartialData& partialData) const {
	static Unique uniqueBindBuffer = UniqueType<IShader::BindBuffer>::Get();
	static Unique uniqueBindTexture = UniqueType<IShader::BindTexture>::Get();

	std::vector<uint32_t> starts(bufferData.size(), ~(uint32_t)0);

	for (uint32_t i = 0; i < parameters.size(); i++) {
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
	for (uint32_t i = 0; i < parameters.size(); i++) {
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
			const PassBase::Parameter* m = nullptr;
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
			const PassBase::Parameter& parameter = m != nullptr && *m ? *m : updater[byteName];

			if (parameter) {
				const_cast<PassBase::Parameter&>(parameter).internalAddress = (void*)((size_t)ptr - (size_t)base);
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
		for (uint32_t i = 0; i < reflector.outputs.size(); i++) {
			Parameter& parameter = reflector.outputs[i];
			sum += (uint32_t)safe_cast<uint32_t>(parameter.type->GetSize());
		}
	
		// check completeness
		assert(sum == reflector.outputs[0].stride); // must provide all segments by now
	}

	particalUpdater.parameters = std::move(reflector.outputs);
}

