// SnowyStream.h
// By PaintDream (paintdream@paintdream.com)
// 2015-1-8
//

#ifndef __SNOWYSTREAM_H__
#define __SNOWYSTREAM_H__

#include "../../General/Interface/Interfaces.h"
#include <string>
#include "File.h"
#include "Zipper.h"
#include "ResourceBase.h"
#include "ResourceManager.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ShaderResource;
		class SnowyStream : public TReflected<SnowyStream, IScript::Library>, public IUniformResourceManager {
		public:
			SnowyStream(Interfaces& interfaces, NsBridgeSunset::BridgeSunset& bs, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const TWrapper<void, const String&>& errorHandler);
			virtual ~SnowyStream();
			IRender::Device* GetRenderDevice() const;
			IRender::Queue* GetResourceQueue() const;

			virtual void TickDevice(IDevice& device);
			virtual void Initialize() override;
			virtual void Uninitialize() override;
			virtual void Reset();
			virtual Interfaces& GetInterfaces() const;
			virtual TShared<ResourceBase> CreateResource(const String& location, const String& extension = "", bool openExisting = true, Tiny::FLAG flag = 0, IStreamBase* sourceStream = nullptr) override;
			virtual bool PersistResource(TShared<ResourceBase> resource, const String& extension = "") override;
			virtual bool MapResource(TShared<ResourceBase> resource, const String& extension = "") override;
			virtual void UnmapResource(TShared<ResourceBase> resource) override;
			virtual bool RegisterResourceManager(Unique unique, ResourceManager* resourceManager);
			virtual bool RegisterResourceSerializer(Unique unique, const String& extension, ResourceSerializerBase* serializer);

		public:
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			void RequestNewResource(IScript::Request& request, const String& path, const String& expectedResType, bool createAlways);
			void RequestNewResourcesAsync(IScript::Request& request, std::vector<String>& pathList, String& expectedResType, IScript::Request::Ref callback);
			void RequestLoadExternalResourceData(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& externalPath);
			void RequestInspectResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource);
			void RequestPersistResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& extension);
			void RequestMapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& extension);
			void RequestCloneResource(IScript::Request& request, IScript::Delegate<ResourceBase>, const String& path);
			void RequestUnmapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource);
			void RequestCompressResourceAsync(IScript::Request& request, IScript::Delegate<ResourceBase> resource, String& compressType, IScript::Request::Ref callback);
			void RequestImportResourceConfig(IScript::Request& request, std::vector<std::pair<String, String> >& version);
			void RequestExportResourceConfig(IScript::Request& request);

			void RequestNewZipper(IScript::Request& request, const String& path);
			void RequestPostZipperData(IScript::Request& request, IScript::Delegate<Zipper> zipper, const String& path, const String& data);
			void RequestWriteZipper(IScript::Request& request, IScript::Delegate<File> file, IScript::Delegate<Zipper> zipper);

			void RequestNewFile(IScript::Request& request, const String& path, bool write);
			void RequestDeleteFile(IScript::Request& request, const String& path);
			void RequestFileExists(IScript::Request& request, const String& path);
			void RequestFlushFile(IScript::Request& request, IScript::Delegate<File> file);
			void RequestReadFile(IScript::Request& request, IScript::Delegate<File> file, int64_t length, IScript::Request::Ref callback);
			void RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file);
			void RequestGetFileLastModifiedTime(IScript::Request& request, IScript::Delegate<File> file);
			void RequestWriteFile(IScript::Request& request, IScript::Delegate<File> file, const String& content, IScript::Request::Ref callback);
			void RequestCloseFile(IScript::Request& request, IScript::Delegate<File> file);
			void RequestSeekFile(IScript::Request& request, IScript::Delegate<File> file, const String& type, int64_t offset);
			void RequestQueryFiles(IScript::Request& request, const String& path);
			void RequestFetchFileData(IScript::Request& request, const String& path);

			void RequestSetShaderResourceCode(IScript::Request& request, IScript::Delegate<ShaderResource> shaderResource, const String& stage, const String& text, const std::vector<std::pair<String, String> >& config);
			void RequestSetShaderResourceInput(IScript::Request& request, IScript::Delegate<ShaderResource> shaderResource, const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config);
			void RequestSetShaderResourceComplete(IScript::Request& request, IScript::Delegate<ShaderResource> shaderResource);

		public:
			void RegisterBuiltinPasses();
			template <class T>
			bool RegisterResourceSerializer(const String& extension, T& device, ResourceSerializerBase* serializer, void* context) {
				Unique unique = UniqueType<T>::Get();
				if (!RegisterResourceSerializer(unique, extension, serializer)) {
					return false;
				}

				std::map<Unique, TShared<ResourceManager> >::iterator it = resourceManagers.find(unique);
				if (it == resourceManagers.end()) {
					ResourceManager* resourceManager = new DeviceResourceManager<T>(interfaces.thread, *this, &interfaces, device, errorHandler, context);
					RegisterResourceManager(unique, resourceManager);
					resourceManager->ReleaseObject();
				}

				return true;
			}

			static String GetReflectedExtension(Unique unique);

			template <class T>
			void RegisterReflectedSerializer(UniqueType<T> type, typename T::DriverType& device, void* context) {
				String extension = GetReflectedExtension(type.Get());
				ResourceSerializerBase* serializer = new ResourceReflectedSerializer<T>(extension);
				RegisterResourceSerializer(extension, device, serializer, context);
				serializer->ReleaseObject();
			}

			template <class T>
			TShared<T> CreateReflectedResource(UniqueType<T> type, const String& location, bool openExisting = true, Tiny::FLAG flag = 0, IStreamBase* sourceStream = nullptr) {
				return static_cast<T*>(CreateResource(location, GetReflectedExtension(type.Get()), openExisting, flag, sourceStream)());
			}

			void CreateBuiltinResources();

		protected:
			void RegisterReflectedSerializers();
			bool FilterPath(const String& path);
			void CreateBuiltinSolidTexture(const String& path, const UChar4& color);

		protected:
			Interfaces& interfaces;
			NsBridgeSunset::BridgeSunset& bridgeSunset;
			const TWrapper<void, const String&> errorHandler;
			unordered_map<String, std::pair<Unique, TShared<ResourceSerializerBase> > > resourceSerializers;
			std::map<Unique, TShared<ResourceManager> > resourceManagers;
			static String reflectedExtension;
			const TWrapper<IArchive*, IStreamBase&, size_t> subArchiveCreator;

		protected:
			// Device related
			// Render
			IRender::Device* renderDevice;
			IRender::Queue* resourceQueue;
		};

		class MetaResourceExternalPersist : public TReflected<MetaResourceExternalPersist, MetaStreamPersist> {
		public:
			MetaResourceExternalPersist();
			virtual IReflectObject* Clone() const override;

			template <class T, class D>
			inline const MetaResourceExternalPersist& FilterField(T* t, D* d) const {
				const_cast<String&>(uniqueName) = UniqueType<D>::Get()->GetSubName();
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef MetaResourceExternalPersist Type;
			};

			typedef MetaResourceExternalPersist Type;

			virtual bool Read(IStreamBase& streamBase, void* ptr) const override;
			virtual bool Write(IStreamBase& streamBase, const void* ptr) const override;
			virtual const String& GetUniqueName() const override;

		private:
			String uniqueName;
		};
	}
}


#endif // __SNOWYSTREAM_H__
