#include "LeavesExporterPCH.h"
#include "Service.h"
#include "../../../../../Source/General/Driver/Network/LibEvent/ZNetworkLibEvent.h"
#include "../../../../../Source/Core/Driver/Thread/Pthread/ZThreadPthread.h"

namespace PaintsNow {
	namespace NsGalaxyWeaver {
		static ZThreadPthread uniqueThreadApi;
		static ZRemoteFactory remoteFactory;
		static ZNetworkLibEvent libEvent(uniqueThreadApi);
		static ProviderFactory localFactory(uniqueThreadApi, libEvent);

		class UnrealResourceManager : public NsSnowyStream::ResourceManager {
		public:
			UnrealResourceManager(IThread& threadApi, NsSnowyStream::IUniformResourceManager& hostManager) : ResourceManager(threadApi, hostManager, TWrapper<void, const String&>(), nullptr) {}
			virtual Unique GetDeviceUnique() const {
				return UniqueType<UObject>::Get();
			}

			virtual void InvokeAttach(NsSnowyStream::ResourceBase* resource, void* deviceContext) override {}
			virtual void InvokeDetach(NsSnowyStream::ResourceBase* resource, void* deviceContext) override {}
			virtual void InvokeUpload(NsSnowyStream::ResourceBase* resource, void* deviceContext) override {}
			virtual void InvokeDownload(NsSnowyStream::ResourceBase* resource, void* deviceContext) override {}
		};

		class DummyUniformResourceManager : public NsSnowyStream::IUniformResourceManager {
		public:
			virtual TShared<NsSnowyStream::ResourceBase> CreateResource(const String& location, const String& extension = "", bool openExisting = true, Tiny::FLAG flag = 0, IStreamBase* sourceStream = nullptr) override {
				return nullptr;
			}
			virtual bool PersistResource(TShared<NsSnowyStream::ResourceBase> resource, const String& extension = "") override {
				return false;
			}
			virtual bool MapResource(TShared<NsSnowyStream::ResourceBase> resource, const String& extension = "") override {
				return false;
			}

			virtual void UnmapResource(TShared<NsSnowyStream::ResourceBase> resource) override {

			}
		};

		void Service::StatusHandler(IScript::Request& request, bool isServer, ZRemoteProxy::STATUS status, const String& message) {
			if (status == ZRemoteProxy::CONNECTED) {
				// Check Version
				IScript::Request::Ref global = request.Load("Global", "Initialize");
				request.QueryInterface(Wrap(this, &Service::OnInitializeQuery), remoteFactory, global);
			}
		}

		void Service::Initialize(ISceneExplorer* se) {
			sceneExp = se;
			static DummyUniformResourceManager uniformResourceManager;
			resourceManager = std::make_unique<UnrealResourceManager>(uniqueThreadApi, uniformResourceManager);
			remoteProxy = std::make_unique<ZRemoteProxy>(uniqueThreadApi, libEvent, localFactory, "", Wrap(this, &Service::StatusHandler));
			// connecting ...
			remoteProxy->Run();
			// Reconnect("LeavesWingScenePoster");
		}

		void Service::Reconnect(const String& port) {
			mainRequest.reset(remoteProxy->NewRequest(port));
		}

		void Service::Deinitialize() {
			// order assurance 
			mainRequest.release();
			controller.release();
			remoteProxy.release();
		}

		void Service::OnInitializeQuery(IScript::Request& request, IReflectObject& inter, const IScript::Request::Ref& ref) {
			assert(&inter == &remoteFactory);
			request.Push();
			remoteFactory.NewObject(request.Adapt(Wrap(this, &Service::OnCreateWeaverObject)), request,  "Weaver");
			request.Pop();
		}

		void Service::OnCreateWeaverObject(IScript::Request& request, IScript::Request::Ref instance) {
			controller.reset(new Controller(uniqueThreadApi, libEvent, ""));
			request.Push();
			request.QueryInterface(Wrap(this, &Service::OnWeaverObjectQuery), *controller, instance);
			request.Pop();
		}

		void Service::OnWeaverObjectQuery(IScript::Request& request, IReflectObject& inter, const IScript::Request::Ref& ref) {
			// instance: can be used to make rpc calls to remotely created weaver instance
			// Now we just check version
			request.Push();
			controller->CheckVersion(request.Adapt(Wrap(this, &Service::OnCheckVersion)), request);
			request.Pop();
		}

		void Service::OnCheckVersion(IScript::Request& request, String& mainVersion, String& subVersion, String& buildVersion) {
			String str = "OnCheckVersion(): ";
			str += mainVersion + " " + subVersion + " " + buildVersion;
			sceneExp->WriteLog(ISceneExplorer::LOG_TEXT, str.c_str());
		}

		NsSnowyStream::ResourceManager& Service::GetResourceManager() const {
			assert(resourceManager.get() != nullptr);
			return *resourceManager.get();
		}

		bool Service::IsConnected() const {
			return mainRequest != nullptr;
		}

		IScript::Request& Service::GetMainRequest() const {
			assert(mainRequest.get() != nullptr);
			return *mainRequest.get();
		}
	}
}