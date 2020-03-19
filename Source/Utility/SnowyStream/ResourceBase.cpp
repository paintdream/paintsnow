#include "ResourceBase.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

#ifdef _DEBUG
#ifdef _WIN32
#include <Windows.h>
#endif
struct LeakGuard {
	LeakGuard() {
		section.store(0, std::memory_order_relaxed);
	}

	~LeakGuard() {
		char buffer[512];
		for (std::set<SharedTiny*>::iterator it = collection.begin(); it != collection.end(); ++it) {
			SharedTiny* s = *it;
			sprintf(buffer, "Leak object: %p\n", s);
#ifdef _WIN32
			OutputDebugStringA(buffer);
#endif
		}
	}

	void Insert(SharedTiny* s) {
		SpinLock(section);
		collection.insert(s);
		SpinUnLock(section);
	}

	void Remove(SharedTiny* s) {
		SpinLock(section);
		collection.erase(s);
		SpinUnLock(section);
	}

private:
	TAtomic<int32_t> section;
	std::set<SharedTiny*> collection;
};

static LeakGuard leakGuard;
#endif

ResourceBase::ResourceBase(ResourceManager& manager, const ResourceManager::UniqueLocation& id) : BaseClass(Tiny::TINY_UNIQUE | Tiny::TINY_READONLY | Tiny::TINY_ACTIVATED | Tiny::TINY_UPDATING), resourceManager(manager), uniqueLocation(id) {
#ifdef _DEBUG
	leakGuard.Insert(this);
#endif
	mapCount.store(0, std::memory_order_relaxed);
	critical.store(0, std::memory_order_relaxed);
}

ResourceBase::~ResourceBase() {
#ifdef _DEBUG
	leakGuard.Remove(this);
#endif
}

std::pair<uint16_t, uint16_t> ResourceBase::GetProgress() const {
	return std::make_pair(0, 1);
}

ResourceManager& ResourceBase::GetResourceManager() const {
	return resourceManager;
}

bool ResourceBase::IsPrepared() const {
	return !(Flag() & TINY_UPDATING);
}

void ResourceBase::ReleaseObject() {
	// last?
	if (GetExtReferCount() == 0) {
		// no references exist, remove this from resource manager
		if (!(Flag() & RESOURCE_ORPHAN)) {
			resourceManager.Remove(this);
		}
	}

	SharedTiny::ReleaseObject();
}

ResourceManager::UniqueLocation ResourceBase::GenerateRandomLocation(const String& prefix, const void* ptr) {
	std::stringstream ss;
	ss << "[Temporary]/" << prefix << "/" << std::hex << (size_t)ptr;
	return ss.str();
}

uint64_t ResourceBase::GetMemoryUsage() const {
	return sizeof(*this);
}

Unique ResourceBase::GetDeviceUnique() const {
	assert(false);
	return Unique();
}

bool ResourceBase::LoadExternalResource(IStreamBase& streamBase, size_t length) {
	return false; // by default no external resource supported.
}

bool ResourceBase::Compress(const String& compressType) {
	return false; // by default no compression available
}

bool ResourceBase::Map() {
	if (mapCount.fetch_add(1, std::memory_order_relaxed) == 0) {
		return resourceManager.GetUniformResourceManager().MapResource(this);
	} else {
		return true;
	}
}

void ResourceBase::Unmap() {
	if (mapCount.fetch_sub(1, std::memory_order_release) == 1) {
		resourceManager.GetUniformResourceManager().UnmapResource(this);
	}
}

class SearchDependencies : public IReflect {
public:
	SearchDependencies(std::vector<ResourceBase::Dependency>& d) : deps(d), IReflect(true, false) {}

	void AddDependency(TShared<ResourceBase>& resource) {
		if (resource) {
			ResourceBase::Dependency dep;
			dep.key = currentPath;
			dep.value = resource->GetLocation();

			deps.emplace_back(std::move(dep));
		}
	}

	virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
		// scan MetaResourceInternalPersist	
		const MetaResourceInternalPersist* resourcePersist = nullptr;
		while (meta != nullptr) {
			const MetaResourceInternalPersist* persist = meta->GetNode()->QueryInterface(UniqueType<MetaResourceInternalPersist>());
			if (persist != nullptr) {
				resourcePersist = persist;
				break;
			}

			meta = meta->GetNext();
		}

		String savedPath = currentPath;
		currentPath = savedPath + "." + name;

		if (s.IsBasicObject()) {
			if (resourcePersist != nullptr) {
				// Must be TShared<ResourceXXX>
				AddDependency(*reinterpret_cast<TShared<ResourceBase>*>(ptr));
			}
		} else {
			if (s.IsIterator()) {
				IIterator& iterator = static_cast<IIterator&>(s);
				uint32_t index = 0;
				const IReflectObject& prototype = iterator.GetPrototype();
				while (iterator.Next()) {
					std::stringstream ss;
					ss << savedPath << "." << name << "[" + index++ << "]";
					currentPath = ss.str();
					if (!prototype.IsBasicObject()) {
						IReflectObject* p = reinterpret_cast<IReflectObject*>(iterator.Get());
						(*p)(*this);
					} else if (resourcePersist != nullptr) {
						AddDependency(*reinterpret_cast<TShared<ResourceBase>*>(iterator.Get()));
					}
				}
			} else {
				s(*this);
			}
		}

		currentPath = savedPath;
	}

	virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

	String currentPath;
	std::vector<ResourceBase::Dependency>& deps;
};

void ResourceBase::GetDependencies(std::vector<Dependency>& deps) const {
	SearchDependencies searcher(deps);
	(*const_cast<ResourceBase*>(this))(searcher);
}

Unique ResourceBase::GetBaseUnique() const {
	return GetUnique();
}

TObject<IReflect>& ResourceBase::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(uniqueLocation)[Runtime];
	}

	return *this;
}

const String& ResourceBase::GetLocation() const {
	return uniqueLocation;
}

void ResourceBase::SetLocation(const ResourceManager::UniqueLocation& location) {
	uniqueLocation = location;
}

MetaResourceInternalPersist::MetaResourceInternalPersist(ResourceManager& r) : resourceManager(r) {
	uniqueName = GetUnique()->GetSubName() + "(" + r.GetUnique()->GetSubName() + ")";
}

const String& MetaResourceInternalPersist::GetUniqueName() const {
	return uniqueName;
}

bool MetaResourceInternalPersist::Read(IStreamBase& streamBase, void* ptr) const {
	String path;
	if ((streamBase >> path) && !path.empty()) {
		TShared<ResourceBase>& res = *reinterpret_cast<TShared<ResourceBase>*>(ptr);
		ResourceManager& manager = (const_cast<ResourceManager&>(resourceManager));
		IUniformResourceManager& uniformResourceManager = manager.GetUniformResourceManager();
		if (res = uniformResourceManager.CreateResource(path)) {
			res->GetResourceManager().InvokeUpload(res());
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

bool MetaResourceInternalPersist::Write(IStreamBase& streamBase, const void* ptr) const {
	const TShared<ResourceBase>& res = *reinterpret_cast<const TShared<ResourceBase>*>(ptr);
	return streamBase << (res ? res->GetLocation() + "." + res->GetBaseUnique()->GetSubName() : String(""));
}

IReflectObject* MetaResourceInternalPersist::Clone() const {
	return new MetaResourceInternalPersist(*this);
}