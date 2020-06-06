// ResourceBase.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#ifndef __RESOURCEBASE_H__
#define __RESOURCEBASE_H__

#include "../../Core/System/Tiny.h"
#include "ResourceManager.h"

namespace PaintsNow {
	class Interfaces;
	namespace NsSnowyStream {
		class IUniformResourceManager {
		public:
			virtual TShared<ResourceBase> CreateResource(const String& location, const String& extension = "", bool openExisting = true, Tiny::FLAG flag = 0, IStreamBase* sourceStream = nullptr) = 0;
			virtual bool PersistResource(TShared<ResourceBase> resource, const String& extension = "") = 0;
			virtual bool MapResource(TShared<ResourceBase> resource, const String& extension = "") = 0;
			virtual void UnmapResource(TShared<ResourceBase> resource) = 0;
		};

		class ResourceBase : public TReflected<ResourceBase, SharedTiny> {
		public:
			enum {
				RESOURCE_UPLOADED = TINY_CUSTOM_BEGIN << 0,
				RESOURCE_DOWNLOADED = TINY_CUSTOM_BEGIN << 1,
				RESOURCE_STREAM = TINY_CUSTOM_BEGIN << 2,
				RESOURCE_ETERNAL = TINY_CUSTOM_BEGIN << 3,
				RESOURCE_ORPHAN = TINY_CUSTOM_BEGIN << 4,
				RESOURCE_COMPRESSED = TINY_CUSTOM_BEGIN << 5,
				RESOURCE_CUSTOM_BEGIN = TINY_CUSTOM_BEGIN << 6
			};

			static ResourceManager::UniqueLocation GenerateLocation(const String& prefix, const void* ptr);
			typedef Void DriverType;
			ResourceBase(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueLocation);
			virtual ~ResourceBase();
			ResourceManager& GetResourceManager() const;
			const ResourceManager::UniqueLocation& GetLocation() const;
			void SetLocation(const ResourceManager::UniqueLocation& location);

			virtual Unique GetDeviceUnique() const;
			virtual bool LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length);
			virtual bool Compress(const String& compressType);

			struct Dependency {
				String key;
				ResourceManager::UniqueLocation value;
			};

			virtual Unique GetBaseUnique() const;
			virtual void GetDependencies(std::vector<Dependency>& deps) const;
			virtual bool IsPrepared() const;
			virtual std::pair<uint16_t, uint16_t> GetProgress() const;
			virtual void ReleaseObject() override;
			// override them derived from IReflectObject to change behaviors on serialization & deserialization
			// virtual bool operator >> (IStreamBase& stream) const override;
			// virtual bool operator << (IStreamBase& stream) override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual bool Map();
			virtual void Unmap();
			TAtomic<uint32_t> critical;

		protected:
			ResourceManager::UniqueLocation uniqueLocation;
			ResourceManager& resourceManager;
			TAtomic<uint32_t> mapCount;
		};

		template <class T>
		class DeviceResourceBase : public TReflected<DeviceResourceBase<T>, ResourceBase> {
		public:
			typedef TReflected<DeviceResourceBase<T>, ResourceBase> BaseClass;
			typedef T DriverType;
			DeviceResourceBase(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueLocation) : BaseClass(manager, uniqueLocation) {}

			virtual void Download(T& device, void* deviceContext) = 0;
			virtual void Upload(T& device, void* deviceContext) = 0;
			virtual void Attach(T& device, void* deviceContext) = 0;
			virtual void Detach(T& device, void* deviceContext) = 0;
			virtual size_t ReportDeviceMemoryUsage() const { return 0; }

			virtual Unique GetDeviceUnique() const {
				return UniqueType<T>::Get();
			}
		};

		class MetaResourceInternalPersist : public TReflected<MetaResourceInternalPersist, MetaStreamPersist> {
		public:
			MetaResourceInternalPersist(ResourceManager& resourceManager);
			virtual IReflectObject* Clone() const override;

			template <class T, class D>
			inline const MetaResourceInternalPersist& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef MetaResourceInternalPersist Type;
			};

			typedef MetaResourceInternalPersist Type;

			virtual bool Read(IStreamBase& streamBase, void* ptr) const override;
			virtual bool Write(IStreamBase& streamBase, const void* ptr) const override;
			virtual const String& GetUniqueName() const override;

		private:
			ResourceManager& resourceManager;
			String uniqueName;
		};
	}
}

#endif // __RESOURCEBASE_H__
