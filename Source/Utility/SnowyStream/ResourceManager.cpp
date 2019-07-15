#include "ResourceBase.h"
#include "ResourceManager.h"
#include "../../Core/Interface/IArchive.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

static const String uniExtension = ".pod";

ResourceManager::ResourceManager(IThread& threadApi, IUniformResourceManager& hostManager, Interfaces* inters, const TWrapper<void, const String&>& err, void* c) : ISyncObject(threadApi), uniformResourceManager(hostManager), interfaces(inters), errorHandler(err), context(c) {
}

ResourceManager::~ResourceManager() {
	assert(resourceMap.empty());
}

void ResourceManager::Report(const String& err) {
	if (errorHandler)
		errorHandler(err);
}

Interfaces* ResourceManager::GetInterfaces() const {
	return interfaces;
}

void ResourceManager::RemoveAll() {
	DoLock();

	// remove all eternal resources
	unordered_map<UniqueLocation, ResourceBase*> temp;
	std::swap(temp, resourceMap);
	for (unordered_map<UniqueLocation, ResourceBase*>::const_iterator it = temp.begin(); it != temp.end(); ++it) {
		ResourceBase* resource = (*it).second;
		InvokeDetach(resource, GetContext());
		resource->Flag() |= ResourceBase::RESOURCE_ORPHAN;

		if (resource->Flag() & ResourceBase::RESOURCE_ETERNAL) {
			resource->ReleaseObject();
		}
	}

	UnLock();
}

ResourceBase* ResourceManager::Insert(const UniqueLocation& id, ResourceBase* resource) {
	assert(resource != nullptr);

	// allowing anounymous resource
	if (id.empty()) return resource;

	DoLock();
	assert(!id.empty());
	unordered_map<UniqueLocation, ResourceBase*>::iterator it = resourceMap.find(id);
	if (it == resourceMap.end()) {
		resourceMap[id] = resource;
		if (resource->Flag() & ResourceBase::RESOURCE_ETERNAL) {
			resource->ReferenceObject();
		}

		InvokeAttach(resource, GetContext());
	} else {
		resource->ReleaseObject();
		resource = (*it).second;
	}
	UnLock();

	return resource;
}

IUniformResourceManager& ResourceManager::GetUniformResourceManager() {
	return uniformResourceManager;
}

ResourceBase* ResourceManager::LoadExist(const UniqueLocation& id) {
	DoLock();
	unordered_map<UniqueLocation, ResourceBase*>::iterator p = resourceMap.find(id);
	ResourceBase* pointer = nullptr;
	if (p != resourceMap.end()) {
		pointer = (*p).second;
		assert(pointer != nullptr);
		pointer->ReferenceObject();
	}

	UnLock();
	return pointer;
}

void* ResourceManager::GetContext() const {
	return context;
}

void ResourceManager::Remove(ResourceBase* resource) {
	assert(resource != nullptr);
	if (resource->Flag() & (ResourceBase::RESOURCE_ORPHAN | ResourceBase::RESOURCE_ETERNAL))
		return;

	const UniqueLocation& location = resource->GetLocation();
	if (location.empty()) return;

	DoLock();
	// Double check
	assert(resource->GetExtReferCount() == 0);
	if (!location.empty()) {
		resourceMap.erase(location);
	}

	// Parallel bug here.
	InvokeDetach(resource, GetContext());
	resource->Flag() |= ResourceBase::RESOURCE_ORPHAN;
	UnLock();
}

inline ResourceManager::UniqueLocation PathToUniqueID(const String& path) {
	return path;
}

ResourceBase* ResourceSerializerBase::DeserializeFromArchive(ResourceManager& manager, IArchive& archive, const String& path, IFilterBase& protocol, bool openExisting, Tiny::FLAG flag) {
	assert(manager.GetDeviceUnique() == GetDeviceUnique());
	if (manager.GetDeviceUnique() != GetDeviceUnique())
		return nullptr;
		
	ResourceBase* resource = manager.LoadExist(PathToUniqueID(path));
	if (resource != nullptr) {
		return resource;
	}

	size_t length;
	if (openExisting) {
		IStreamBase* stream = archive.Open(path + "." + GetExtension() + uniExtension, false, length);
		if (stream != nullptr) {
			resource = Deserialize(manager, path, protocol, flag, stream);

			if (resource != nullptr) {
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

	return resource;
}

bool ResourceSerializerBase::MapFromArchive(ResourceBase* resource, IArchive& archive, IFilterBase& protocol, const String& path) {
	assert(resource != nullptr);

	size_t length;
	IStreamBase* stream = archive.Open(path + "." + GetExtension() + uniExtension, false, length);
	if (stream != nullptr) {
		bool result = true;
		SpinLock(resource->mapCritical);
		if (!resource->IsMapped()) {
			result = LoadData(resource, protocol, *stream);
			if (result) {
				resource->Map();
			}
		} else {
			resource->Map();
		}
		SpinUnLock(resource->mapCritical);

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
		SpinLock(resource->mapCritical);
		bool result = Serialize(resource, protocol, *stream);
		SpinUnLock(resource->mapCritical);
		stream->ReleaseObject();

		if (!result) {
			archive.Delete(path);
		}

		return result;
	}

	return false;
}

ResourceSerializerBase::~ResourceSerializerBase() {}
