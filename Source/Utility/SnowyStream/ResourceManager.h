// ResourceManager.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-19
//

#pragma once
#include "../../General/Interface/Interfaces.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../Core/Template/TMap.h"
#include "../../Core/Template/TQueue.h"
#include "../../Core/System/Tiny.h"
#include "../../Core/System/ThreadPool.h"
#include "ResourceBase.h"

namespace PaintsNow {
	class IUniformResourceManager;
	template <class T>
	class DeviceResourceBase;
	class ResourceManager : public TReflected<ResourceManager, SharedTiny>, public ISyncObject {
	public:
		ResourceManager(ThreadPool& threadPool, IUniformResourceManager& hostManager, const TWrapper<void, const String&>& errorHandler, void* context);
		~ResourceManager() override;
		void Insert(ResourceBase* resource);
		void Remove(ResourceBase* resource);
		void RemoveAll();
		virtual Unique GetDeviceUnique() const = 0;
		void Report(const String& err);
		void* GetContext() const;
		const String& GetLocationPostfix() const;

		TShared<ResourceBase> LoadExistSafe(const String& uniqueLocation);
		TShared<ResourceBase> LoadExist(const String& uniqueLocation);
		IUniformResourceManager& GetUniformResourceManager();
		ThreadPool& GetThreadPool();

	public:
		virtual void InvokeRefresh(ResourceBase* resource, void* deviceContext = nullptr) = 0;
		virtual void InvokeAttach(ResourceBase* resource, void* deviceContext = nullptr) = 0;
		virtual void InvokeDetach(ResourceBase* resource, void* deviceContext = nullptr) = 0;
		virtual void InvokeUpload(ResourceBase* resource, void* deviceContext = nullptr) = 0;
		virtual void InvokeDownload(ResourceBase* resource, void* deviceContext = nullptr) = 0;

	private:
		ThreadPool& threadPool;
		std::unordered_map<String, ResourceBase*> resourceMap;
		IUniformResourceManager& uniformResourceManager;
		TWrapper<void, const String&> errorHandler;
		TQueueList<TShared<ResourceBase> > runtimeUpdatingResources;
		void* context;
	};

	template <class T>
	class DeviceResourceManager : public ResourceManager {
	public:
		DeviceResourceManager(ThreadPool& tp, IUniformResourceManager& hostManager, T& dev, const TWrapper<void, const String&>& errorHandler, void* context) : ResourceManager(tp, hostManager, errorHandler, context), device(dev) {}
		Unique GetDeviceUnique() const override {
			return UniqueType<T>::Get();
		}

		T& GetDevice() const {
			return device;
		}

		void InvokeRefresh(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Refresh(device, deviceContext != nullptr ? deviceContext : GetContext());
		}

		void InvokeAttach(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			assert(!(resource->Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_ATTACHED));
			resource->Flag().fetch_or(ResourceBase::RESOURCE_ATTACHED, std::memory_order_relaxed);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Attach(device, deviceContext != nullptr ? deviceContext : GetContext());
		}

		void InvokeDetach(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			assert(resource->Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_ATTACHED);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Detach(device, deviceContext != nullptr ? deviceContext : GetContext());
			resource->Flag().fetch_and(~ResourceBase::RESOURCE_ATTACHED, std::memory_order_release);
		}

		void InvokeUpload(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			assert(resource->Flag().load(std::memory_order_acquire) & ResourceBase::TINY_MODIFIED);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Upload(device, deviceContext != nullptr ? deviceContext : GetContext());
		}

		void InvokeDownload(ResourceBase* resource, void* deviceContext) override {
			assert(resource != nullptr);
			DeviceResourceBase<T>* typedResource = static_cast<DeviceResourceBase<T>*>(resource);
			typedResource->Download(device, deviceContext != nullptr ? deviceContext : GetContext());
		}

	protected:
		T& device;
	};

	class ResourceCreator : public TReflected<ResourceCreator, SharedTiny> {
	public:
		~ResourceCreator() override;

		virtual TShared<ResourceBase> Create(ResourceManager& manager, const String& location) = 0;
		virtual Unique GetDeviceUnique() const = 0;
		virtual const String& GetExtension() const = 0;
	};

	template <class T>
	class ResourceReflectedCreator : public ResourceCreator {
	public:
		ResourceReflectedCreator(const String& ext) : extension(ext) {}

	protected:
		const String& GetExtension() const override { return extension; }
		Unique GetDeviceUnique() const override {
			return UniqueType<typename T::DriverType>::Get();
		}

		TShared<ResourceBase> Create(ResourceManager& manager, const String& location) override {
			return TShared<ResourceBase>::From(new T(manager, location));
		}

		String extension;
	};
}

