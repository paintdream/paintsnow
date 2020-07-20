#include "ZDynamicObject.h"
#include <sstream>
using namespace PaintsNow;

ZDynamicUniqueAllocator::ZDynamicUniqueAllocator() {}

ZDynamicInfo::Field::Field() : controller(nullptr) {
	reflectable = 0;
	offset = 0;
}

IReflectObject* ZDynamicInfo::Create() const {
	assert(Alignment(sizeof(ZDynamicObject)) >= sizeof(size_t));
	size_t s = sizeof(ZDynamicObject) + size;
	char* buffer = new char[s];
	memset(buffer, 0, s);
	ZDynamicObject* proxy = new (buffer) ZDynamicObject(const_cast<ZDynamicInfo*>(this));
	return proxy;
}

const ZDynamicInfo::Field* ZDynamicInfo::operator [] (const String& key) const {
	std::map<String, uint32_t>::const_iterator it = mapNameToField.find(key);
	return it == mapNameToField.end() ? (ZDynamicInfo::Field*)nullptr : &fields[it->second];
}

ZDynamicObject::ZDynamicObject(ZDynamicInfo* info) : dynamicInfo(info) {
	std::vector<ZDynamicInfo::Field>& fields = dynamicInfo->fields;
	char* base = (char*)this + sizeof(*this);
	for (size_t i = 0; i < fields.size(); i++) {
		ZDynamicInfo::Field& field = fields[i];
		char* ptr = base + field.offset;
		if (field.type->GetAllocator() == dynamicInfo->GetAllocator()) {
			// dynamic-created class?
			assert(field.controller == nullptr);
			new (ptr) ZDynamicObject(dynamicInfo);
		} else if (field.controller != nullptr) {
			field.controller->Creator(ptr);
		}
	}
}

ZDynamicObject::~ZDynamicObject() {
	std::vector<ZDynamicInfo::Field>& fields = dynamicInfo->fields;
	char* base = (char*)this + sizeof(*this);
	for (size_t i = 0; i < fields.size(); i++) {
		ZDynamicInfo::Field& field = fields[i];
		char* ptr = base + field.offset;
		if (field.type->GetAllocator() == dynamicInfo->GetAllocator()) {
			assert(field.controller == nullptr);
			ZDynamicObject* proxy = reinterpret_cast<ZDynamicObject*>(ptr);
			proxy->~ZDynamicObject();
		} else if (field.controller != nullptr) {
			field.controller->Deletor(ptr);
		}
	}

#ifdef _DEBUG
	dynamicInfo = nullptr;
#endif
}

void ZDynamicObject::ReleaseObject() {
	// call destructor manually
	this->~ZDynamicObject();
	delete[] (char*)this;
}

ZDynamicVector::Iterator::Iterator(ZDynamicVector* vec) : base(vec), i(0) {}

void ZDynamicVector::Iterator::Initialize(size_t c) {
	i = 0;
	base->Reinit(base->unique, base->memController, c, base->reflectable);
}

size_t ZDynamicVector::Iterator::GetTotalCount() const {
	return base->count;
}

Unique ZDynamicVector::Iterator::GetPrototypeUnique() const {
	return base->unique;
}

Unique ZDynamicVector::Iterator::GetPrototypeReferenceUnique() const {
	return base->unique;
}

static IReflectObject dummyObject((int)0);

const IReflectObject& ZDynamicVector::Iterator::GetPrototype() const {
	return base->count == 0 || !base->reflectable ? dummyObject : *reinterpret_cast<const IReflectObject*>(base->buffer);
}

void* ZDynamicVector::Iterator::Get() {
	return (char*)base->buffer + (i - 1) * base->unique->GetSize();
}

bool ZDynamicVector::Iterator::Next() {
	if (i >= base->count) {
		return false;
	}

	i++;
	return true;
}

IIterator* ZDynamicVector::Iterator::New() const {
	return new ZDynamicVector::Iterator(base);
}

void ZDynamicVector::Iterator::Attach(void* p) {
	base = reinterpret_cast<ZDynamicVector*>(p);
	i = 0;
}

bool ZDynamicVector::Iterator::IsLayoutLinear() const {
	return true;
}

bool ZDynamicVector::Iterator::IsLayoutPinned() const {
	return false;
}

void* ZDynamicVector::Iterator::GetHost() const {
	return base;
}

TObject<IReflect>& ZDynamicObject::operator () (IReflect& reflect) {
	reflect.Class(*this, dynamicInfo, dynamicInfo->GetName().c_str(), "PaintsNow", nullptr);

	if (reflect.IsReflectProperty()) {
		char* base = (char*)this + sizeof(*this);
		std::vector<ZDynamicInfo::Field>& fields = dynamicInfo->fields;
		for (size_t i = 0; i < fields.size(); i++) {
			ZDynamicInfo::Field& field = fields[i];
			char* ptr = base + field.offset;
			IReflectObject* reflectObject = reinterpret_cast<IReflectObject*>(ptr);
			if (field.type == UniqueType<ZDynamicVector>::Get()) {
				ZDynamicVector::Iterator iterator(static_cast<ZDynamicVector*>(reflectObject));
				reflect.Property(iterator, field.type, field.type, field.name.c_str(), (char*)this, ptr, nullptr);
			} else {
				reflect.Property(field.reflectable ? dummyObject : *reflectObject, field.type, field.type, field.name.c_str(), (char*)(this), ptr, nullptr);
			}
		}
	}

	return *this;
}

ZDynamicInfo* ZDynamicUniqueAllocator::Create(const String& name, size_t size) {
	assert(false);
	return nullptr;
}

ZDynamicInfo* ZDynamicUniqueAllocator::Get(const String& name) {
	std::map<String, ZDynamicInfo>::iterator it = mapType.find(name);
	return it == mapType.end() ? nullptr : &it->second;
}

ZDynamicInfo* ZDynamicUniqueAllocator::AllocFromDescriptor(const String& name, const std::vector<ZDynamicInfo::Field>& descriptors) {
	// Compose new name
	String desc;
	String allNames;
	size_t maxSize = (size_t)-1;
	std::vector<ZDynamicInfo::Field> fields = descriptors;
	size_t lastOffset = 0;
	size_t maxAlignment = sizeof(ZDynamicObject);

	for (size_t k = 0; k < fields.size(); k++) {
		if (k != 0) desc += ", ";
		ZDynamicInfo::Field& p = fields[k];
		Unique info = p.type;
		maxSize = Min(maxSize, info->GetSize());

		desc += p.name + ": " + info->GetName();
		allNames += info->GetName();
		size_t s = Alignment(info->GetSize());
		maxAlignment = Max(maxAlignment, s);
		while (lastOffset != 0 && Alignment(lastOffset) < s) {
			lastOffset += Alignment(lastOffset);
		}

		p.offset = lastOffset;
		lastOffset += info->GetSize();
	}

	std::stringstream ss;
	ss << name.c_str() << "{" << descriptors.size() << "-" << HashBuffer(allNames.c_str(), allNames.size()) << "-" << HashBuffer(desc.c_str(), desc.size()) << "}";

	String newName = ss.str();
	std::map<String, ZDynamicInfo>::iterator it = mapType.find(newName);
	if (it != mapType.end()) {
		// assert(false); // Performance warning: should check it before calling me!
		return &it->second;
	} else {
		ZDynamicInfo& info = mapType[newName];
		info.SetAllocator(this);
		info.SetName(newName);
		std::sort(fields.begin(), fields.end());
		for (size_t i = 0; i < fields.size(); i++) {
			info.mapNameToField[fields[i].name] = safe_cast<uint32_t>(i);
		}

		while (lastOffset != 0 && Alignment(lastOffset) < maxAlignment) {
			lastOffset += Alignment(lastOffset);
		}

		info.SetSize(fields.empty() ? sizeof(size_t) : lastOffset);
		std::swap(info.fields, fields);

		return &info;
	}
}

ZDynamicInfo* ZDynamicObject::GetDynamicInfo() const {
	return dynamicInfo;
}

ZDynamicObject& ZDynamicObject::operator = (const ZDynamicObject& rhs) {
	if (dynamicInfo == rhs.dynamicInfo && this != &rhs) {
		std::vector<ZDynamicInfo::Field>& fields = dynamicInfo->fields;
		const char* rbase = (const char*)&rhs + sizeof(*this);
		for (size_t i = 0; i < fields.size(); i++) {
			ZDynamicInfo::Field& field = fields[i];
			Set(field, rbase);
		}
	}

	return *this;
}

void ZDynamicObject::Set(const ZDynamicInfo::Field& field, const void* value) {
	char* buffer = (char*)this + sizeof(*this) + field.offset;
	if (buffer == value) return;

	if (field.type->GetAllocator() == dynamicInfo->GetAllocator()) {
		const ZDynamicObject* src = reinterpret_cast<const ZDynamicObject*>(value);
		ZDynamicObject* dst = reinterpret_cast<ZDynamicObject*>(buffer);
		*dst = *src;
	} else if (field.controller != nullptr) {
		field.controller->Assigner(buffer, value);
	} else {
		memcpy(buffer, value, field.type->GetSize());
	}
}

void* ZDynamicObject::Get(const ZDynamicInfo::Field& field) const {
	return (char*)this + sizeof(*this) + field.offset;
}

ZDynamicObjectWrapper::ZDynamicObjectWrapper(ZDynamicUniqueAllocator& allocator) : uniqueAllocator(uniqueAllocator), dynamicObject(nullptr) {}

ZDynamicObjectWrapper::~ZDynamicObjectWrapper() {
	if (dynamicObject != nullptr) {
		dynamicObject->ReleaseObject();
	}
}

TObject<IReflect>& ZDynamicObjectWrapper::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

ZDynamicVector::ZDynamicVector(Unique type, ZDynamicInfo::MemController* mc, size_t c, bool r) : unique(type), memController(mc), reflectable(r), count(safe_cast<uint32_t>(c)), buffer(nullptr) {
	Init();
}

void ZDynamicVector::Init() {
	size_t size = unique->GetSize();
	if (count != 0) {
		assert(buffer == nullptr);
		buffer = new char[size * count];

		// initialize them
		if (memController != nullptr) {
			char* base = reinterpret_cast<char*>(buffer);
			for (size_t k = 0; k < count; k++) {
				memController->Creator(base + k * size);
			}
		}
	}
}

ZDynamicVector::~ZDynamicVector() {
	Cleanup();
}

void ZDynamicVector::Reinit(Unique u, ZDynamicInfo::MemController* mc, size_t n, bool r) {
	Cleanup();
	reflectable = r;
	unique = u;
	memController = mc;	
	count = n;
	Init();
}

void ZDynamicVector::Set(size_t i , const void* value) {
	memController->Assigner(reinterpret_cast<char*>(buffer) + i * unique->GetSize(), value);
}

void* ZDynamicVector::Get(size_t i) const {
	return reinterpret_cast<char*>(buffer) + i * unique->GetSize();
}

void ZDynamicVector::Cleanup() {
	if (buffer != nullptr) {
		if (memController != nullptr) {
			size_t size = unique->GetSize();
			char* base = reinterpret_cast<char*>(buffer);
			for (size_t k = 0; k < count; k++) {
				memController->Deletor(base + k * size);
			}
		}

		delete[] reinterpret_cast<char*>(buffer);
		buffer = nullptr;
	}
}

ZDynamicVector& ZDynamicVector::operator = (const ZDynamicVector& vec) {
	if (this != &vec) {
		Cleanup();
		// copy info
		assert(buffer == nullptr);
		unique = vec.unique;
		memController = vec.memController;
		count = vec.count;

		size_t size = unique->GetSize();
		if (count != 0) {
			buffer = new char[size * count];

			// initialize them
			if (memController != nullptr) {
				char* base = reinterpret_cast<char*>(buffer);
				const char* src = reinterpret_cast<const char*>(vec.buffer);
				for (size_t k = 0; k < count; k++) {
					memController->Creator(base + k * size);
					memController->Assigner(base + k * size, src + k * size);
				}
			}
		}
	}

	return *this;
}

void ZDynamicVector::VectorCreator(void* buffer) {
	new (buffer) ZDynamicVector(UniqueType<int>::Get(), nullptr, 0, false);
}

void ZDynamicVector::VectorDeletor(void* buffer) {
	reinterpret_cast<ZDynamicVector*>(buffer)->~ZDynamicVector();
}

void ZDynamicVector::VectorAssigner(void* dst, const void* src) {
	*reinterpret_cast<ZDynamicVector*>(dst) = *reinterpret_cast<const ZDynamicVector*>(src);
}

ZDynamicInfo::MemController& ZDynamicVector::GetVectorController() {
	static ZDynamicInfo::MemController controller = {
		&ZDynamicVector::VectorCreator,
		&ZDynamicVector::VectorDeletor,
		&ZDynamicVector::VectorAssigner,
	};

	return controller;
}