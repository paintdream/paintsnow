// ResourceManager.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-19
//

#pragma once
#include "../../General/Interface/Interfaces.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../Core/Template/TMap.h"
#include "../../Core/System/Tiny.h"
#include "ResourceBase.h"

namespace PaintsNow {
	class IUniformResourceManager;
	template <class T>
	class DeviceResourceBase;
	class ResourceManager : public TReflected<ResourceManager, SharedTiny>, public ISyncObject {
	public:
		ResourceManager(IThread& threadApi, IUniformResourceManager& hostManager, const TWrapper<void, const String&>& errorHandler, void* context);
		virtual ~ResourceManager();
		void Insert(TShared<ResourceBase> resource);
		void Remove(TShared<ResourceBase> resource);
		void RemoveAll();
		virtual Unique GetDeviceUnique() const = 0;
		void Report(const String& err);
		void* GetContext() const;

		TShared<ResourceBase> SafeLoadExist(const String& uniqueLocation);
		TShared<ResourceBase> LoadExist(const String& uniqueLocation);
		IUniformResourceManager& GetUniformResourceManager();

	public:
		virtual void InvokeAttach(ResourceBase* resource, void* deviceContext = nullptr) = 0;
		virtual void InvokeDetach(ResourceBase* resource, void* deviceContext = nullptr) = 0;
		virtual void InvokeUpload(ResourceBase* resource, void* deviceContext = nullptr) = 0;
		virtual void InvokeDownload(ResourceBase* resource, void* deviceContext = nullptr) = 0;

	private:
		std::unordered_map<String, ResourceBase*> resourceMap;
		IUniformResourceManager& uniformResourceManager;
		Interfaces* interfaces;
		TWrapper<void, const String&> errorHandler;
		void* context;
	};

	template <class T>
	class DeviceResourceManager : public ResourceManager {
	public:
		DeviceResourceManager(IThread& threadApi, IUniformResourceManager& hostManager, T& dev, const TWrapper<void, const String&>& errorHandler, void* context) : ResourceManager(threadApi, hostManager, errorHandler, context), device(dev) {}
		virtual Unique GetDeviceUnique() const {
			return UniqueType<T>::Get();
		}

		virtual void InvokeAttach(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			assert(!(resource->Flag() & ResourceBase::RESOURCE_ATTACHED));
			resource->Flag().fetch_or(ResourceBase::RESOURCE_ATTACHED, std::memory_order_acquire);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Attach(device, deviceContext != nullptr ? deviceContext : GetContext());
		}

		virtual void InvokeDetach(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			assert(resource->Flag() & ResourceBase::RESOURCE_ATTACHED);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Detach(device, deviceContext != nullptr ? deviceContext : GetContext());
			resource->Flag().fetch_and(~ResourceBase::RESOURCE_ATTACHED, std::memory_order_release);
		}

		virtual void InvokeUpload(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			assert(resource->Flag() & ResourceBase::TINY_MODIFIED);
			resource->Flag().fetch_and(~ResourceBase::RESOURCE_UPLOADED, std::memory_order_release);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Upload(device, deviceContext != nullptr ? deviceContext : GetContext());
		}

		virtual void InvokeDownload(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			resource->Flag().fetch_and(~ResourceBase::RESOURCE_DOWNLOADED, std::memory_order_release);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Download(device, deviceContext != nullptr ? deviceContext : GetContext());
		}

	protected:
		T& device;
	};

	class ResourceSerializerBase : public TReflected<ResourceSerializerBase, SharedTiny> {
	public:
		virtual ~ResourceSerializerBase();
		TShared<ResourceBase> DeserializeFromArchive(ResourceManager& manager, IArchive& archive, const String& path, IFilterBase& protocol, bool openExisting, Tiny::FLAG flag);
		bool MapFromArchive(ResourceBase* resource, IArchive& archive, IFilterBase& protocol, const String& path);
		bool SerializeToArchive(ResourceBase* resource, IArchive& archive, IFilterBase& protocol, const String& path);

		virtual TShared<ResourceBase> Deserialize(ResourceManager& manager, const String& id, IFilterBase& protocol, Tiny::FLAG flag, IStreamBase* stream) = 0;
		virtual bool Serialize(ResourceBase* res, IFilterBase& protocol, IStreamBase& stream) = 0;
		virtual bool LoadData(ResourceBase* res, IFilterBase& protocol, IStreamBase& stream) = 0;
		virtual Unique GetDeviceUnique() const = 0;
		virtual const String& GetExtension() const = 0;
	};

	template <class T>
	class ResourceReflectedSerializer : public ResourceSerializerBase {
	public:
		ResourceReflectedSerializer(const String& ext) : extension(ext) {}
	protected:
		virtual const String& GetExtension() const { return extension; }
		virtual Unique GetDeviceUnique() const {
			return UniqueType<typename T::DriverType>::Get();
		}

		virtual bool LoadData(ResourceBase* res, IFilterBase& protocol, IStreamBase& stream) {
			IStreamBase* filter = protocol.CreateFilter(stream);
			assert(filter != nullptr);
			bool success = *filter >> *res;
			filter->ReleaseObject();
			return success;
		}

		virtual TShared<ResourceBase> Deserialize(ResourceManager& manager, const String& id, IFilterBase& protocol, Tiny::FLAG flag, IStreamBase* stream) {
			TShared<T> object = TShared<T>::From(new T(manager, id));

			if (stream != nullptr) {
				if (LoadData(object(), protocol, *stream)) {
					if (flag != 0) object->Flag().fetch_or(flag, std::memory_order_release);

					manager.DoLock();
					manager.Insert(object());
					manager.UnLock();
				} else {
					object = nullptr;
				}
			} else {
				object->Flag().fetch_or(flag, std::memory_order_release);

				manager.DoLock();
				manager.Insert(object());
				manager.UnLock();
			}

			return object();
		}

		virtual bool Serialize(ResourceBase* object, IFilterBase& protocol, IStreamBase& stream) {
			IStreamBase* filter = protocol.CreateFilter(stream);
			assert(filter != nullptr);
			bool state = *filter << *object;
			filter->ReleaseObject();

			return state;
		}

		String extension;
	};
}

