#include "ResourceBase.h"
#include "ResourceManager.h"
#include "../../Core/Interface/IArchive.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

static const String uniExtension = ".pod";

ResourceManager::ResourceManager(IThread& threadApi, IUniformResourceManager& hostManager, const TWrapper<void, const String&>& err, void* c) : ISyncObject(threadApi), uniformResourceManager(hostManager), errorHandler(err), context(c) {
}

ResourceManager::~ResourceManager() {
	assert(resourceMap.empty());
}

void ResourceManager::Report(const String& err) {
	if (errorHandler)
		errorHandler(err);
}

void ResourceManager::RemoveAll() {
	assert(GetLockCount() != 0);

	// remove all eternal resources
	unordered_map<UniqueLocation, ResourceBase*> temp;
	std::swap(temp, resourceMap);

	std::vector<ResourceBase*> externals;
	for (unordered_map<UniqueLocation, ResourceBase*>::const_iterator it = temp.begin(); it != temp.end(); ++it) {
		ResourceBase* resource = (*it).second;
		InvokeDetach(resource, GetContext());
		resource->Flag().fetch_or(ResourceBase::RESOURCE_ORPHAN, std::memory_order_acquire);

		if (resource->Flag() & ResourceBase::RESOURCE_ETERNAL) {
			externals.emplace_back(resource);
		}
	}

	for (size_t i = 0; i < externals.size(); i++) {
		externals[i]->ReleaseObject();
	}
}

void ResourceManager::Insert(TShared<ResourceBase> resource) {
	assert(GetLockCount() != 0);
	assert(resource != nullptr);
	assert(resource->Flag() & ResourceBase::RESOURCE_ORPHAN);

	// allowing anounymous resource
	const UniqueLocation& id = resource->GetLocation();
	if (id.empty()) return;

	assert(!id.empty());
	assert(&resource->GetResourceManager() == this);
	assert(resourceMap.find(id) == resourceMap.end());
	resource->Flag().fetch_and(~ResourceBase::RESOURCE_ORPHAN, std::memory_order_acquire);

	resourceMap[id] = resource();

	if (resource->Flag() & ResourceBase::RESOURCE_ETERNAL) {
		resource->ReferenceObject();
	}

	InvokeAttach(resource(), GetContext());
}

IUniformResourceManager& ResourceManager::GetUniformResourceManager() {
	return uniformResourceManager;
}

TShared<ResourceBase> ResourceManager::LoadExist(const UniqueLocation& id) {
	assert(GetLockCount() != 0);
	unordered_map<UniqueLocation, ResourceBase*>::iterator p = resourceMap.find(id);
	ResourceBase* pointer = nullptr;
	if (p != resourceMap.end()) {
		pointer = (*p).second;
		assert(pointer != nullptr);
	}

	return pointer;
}

TShared<ResourceBase> ResourceManager::SafeLoadExist(const UniqueLocation& id) {
	assert(GetLockCount() == 0);
	DoLock();
	TShared<ResourceBase> pointer = LoadExist(id);
	UnLock();

	return pointer;
}

void* ResourceManager::GetContext() const {
	return context;
}

void ResourceManager::Remove(TShared<ResourceBase> resource) {
	assert(GetLockCount() != 0);
	assert(resource != nullptr);
	if (resource->Flag() & (ResourceBase::RESOURCE_ORPHAN | ResourceBase::RESOURCE_ETERNAL))
		return;

	const UniqueLocation& location = resource->GetLocation();
	if (location.empty()) return;

	// Double check
	if (!location.empty()) {
		resourceMap.erase(location);
	}

	// Parallel bug here.
	InvokeDetach(resource(), GetContext());
	resource->Flag().fetch_or(ResourceBase::RESOURCE_ORPHAN, std::memory_order_acquire);
}

inline ResourceManager::UniqueLocation PathToUniqueID(const String& path) {
	return path;
}

TShared<ResourceBase> ResourceSerializerBase::DeserializeFromArchive(ResourceManager& manager, IArchive& archive, const String& path, IFilterBase& protocol, bool openExisting, Tiny::FLAG flag) {
	assert(manager.GetDeviceUnique() == GetDeviceUnique());
	if (manager.GetDeviceUnique() != GetDeviceUnique())
		return nullptr;
	
	manager.DoLock();

	TShared<ResourceBase> resource = manager.LoadExist(PathToUniqueID(path));
	if (resource) {
		manager.UnLock();
		return resource;
	}

	size_t length;
	if (openExisting) {
		IStreamBase* stream = archive.Open(path + "." + GetExtension() + uniExtension, false, length);
		if (stream != nullptr) {
			resource = Deserialize(manager, path, protocol, flag, stream);

			if (resource) {
				if (!(resource->Flag() & ResourceBase::RESOURCE_STREAM)) {
					stream->ReleaseObject();
				}
			} else {
				stream->ReleaseObject();
				assert(false);
			}
		}
	} else {
		resource = Deserialize(manager, path, protocol, flag, nullptr);
	}

	manager.UnLock();
	return resource;
}

bool ResourceSerializerBase::MapFromArchive(ResourceBase* resource, IArchive& archive, IFilterBase& protocol, const String& path) {
	assert(resource != nullptr);

	size_t length;
	IStreamBase* stream = archive.Open(path + "." + GetExtension() + uniExtension, false, length);
	if (stream != nullptr) {
		bool result = true;
		SpinLock(resource->critical);
		result = LoadData(resource, protocol, *stream);
		SpinUnLock(resource->critical);

		stream->ReleaseObject();
		return result;
	}

	return false;
}

bool ResourceSerializerBase::SerializeToArchive(ResourceBase* resource, IArchive& archive, IFilterBase& protocol, const String& path) {
	assert(resource != nullptr);

	size_t length;
	IStreamBase* stream = archive.Open(path + "." + GetExtension() + uniExtension, true, length);
	if (stream != nullptr) {
		SpinLock(resource->critical);
		bool result = Serialize(resource, protocol, *stream);
		SpinUnLock(resource->critical);
		stream->ReleaseObject();

		if (!result) {
			archive.Delete(path);
		}

		return result;
	}

	return false;
}

ResourceSerializerBase::~ResourceSerializerBase() {}
