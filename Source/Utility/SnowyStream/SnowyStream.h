// SnowyStream.h
// By PaintDream (paintdream@paintdream.com)
// 2015-1-8
//

#pragma once
#include "../../General/Interface/Interfaces.h"
#include <string>
#include "File.h"
#include "Zipper.h"
#include "ResourceBase.h"
#include "ResourceManager.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	class ShaderResource;
	class SnowyStream : public TReflected<SnowyStream, IScript::Library>, public IUniformResourceManager {
	public:
		SnowyStream(Interfaces& interfaces, BridgeSunset& bs, const TWrapper<IArchive*, IStreamBase&, size_t>& subArchiveCreator, const TWrapper<void, const String&>& errorHandler);
		~SnowyStream() override;
		IRender::Device* GetRenderDevice() const;
		IRender::Queue* GetResourceQueue();

		void TickDevice(IDevice& device) override;
		void Initialize() override;
		void Uninitialize() override;
		void Reset();

		virtual Interfaces& GetInterfaces() const;
		TShared<ResourceBase> CreateResource(const String& location, const String& extension = "", bool openExisting = true, Tiny::FLAG flag = 0, IStreamBase* sourceStream = nullptr) override;
		bool PersistResource(const TShared<ResourceBase>& resource, const String& extension = "") override;
		bool MapResource(const TShared<ResourceBase>& resource, const String& extension = "") override;
		void UnmapResource(const TShared<ResourceBase>& resource) override;
		virtual bool RegisterResourceManager(Unique unique, ResourceManager* resourceManager);
		virtual bool RegisterResourceSerializer(Unique unique, const String& extension, ResourceSerializerBase* serializer);

	public:
		TObject<IReflect>& operator () (IReflect& reflect) override;
		TShared<ResourceBase> RequestNewResource(IScript::Request& request, const String& path, const String& expectedResType, bool createAlways);
		void RequestNewResourcesAsync(IScript::Request& request, std::vector<String>& pathList, String& expectedResType, IScript::Request::Ref callbackStep, IScript::Request::Ref callbackComplete);
		void RequestLoadExternalResourceData(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& externalPath);
		void RequestInspectResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource);
		void RequestPersistResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& extension);
		void RequestMapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& extension);
		TShared<ResourceBase> RequestCloneResource(IScript::Request& request, IScript::Delegate<ResourceBase>, const String& path);
		void RequestUnmapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource);
		void RequestCompressResourceAsync(IScript::Request& request, IScript::Delegate<ResourceBase> resource, String& compressType, IScript::Request::Ref callback);

		TShared<Zipper> RequestNewZipper(IScript::Request& request, const String& path);
		void RequestPostZipperData(IScript::Request& request, IScript::Delegate<Zipper> zipper, const String& path, const String& data);
		void RequestWriteZipper(IScript::Request& request, IScript::Delegate<File> file, IScript::Delegate<Zipper> zipper);

		void RequestParseJson(IScript::Request& request, const String& str);

		TShared<File> RequestNewFile(IScript::Request& request, const String& path, bool write);
		void RequestDeleteFile(IScript::Request& request, const String& path);
		void RequestFileExists(IScript::Request& request, const String& path);
		void RequestFlushFile(IScript::Request& request, IScript::Delegate<File> file);
		void RequestReadFile(IScript::Request& request, IScript::Delegate<File> file, int64_t length, IScript::Request::Ref callback);
		uint64_t RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file);
		uint64_t RequestGetFileLastModifiedTime(IScript::Request& request, IScript::Delegate<File> file);
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
				ResourceManager* resourceManager = new DeviceResourceManager<T>(interfaces.thread, *this, device, errorHandler, context);
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
		BridgeSunset& bridgeSunset;
		const TWrapper<void, const String&> errorHandler;
		std::unordered_map<String, std::pair<Unique, TShared<ResourceSerializerBase> > > resourceSerializers;
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
		IReflectObject* Clone() const override;

		template <class T, class D>
		inline const MetaResourceExternalPersist& FilterField(T* t, D* d) const {
			const_cast<String&>(uniqueName) = UniqueType<D>::Get()->GetBriefName();
			return *this; // do nothing
		}

		template <class T, class D>
		struct RealType {
			typedef MetaResourceExternalPersist Type;
		};

		typedef MetaResourceExternalPersist Type;

		bool Read(IStreamBase& streamBase, void* ptr) const override;
		bool Write(IStreamBase& streamBase, const void* ptr) const override;
		const String& GetUniqueName() const override;

	private:
		String uniqueName;
	};
}
