// Service.h
// By PaintDream (paintdream@paintdream.com)
// 2018-2-9
//

#ifndef __SERVICE_H__
#define __SERVICE_H__

#include "Provider.h"
#include "ISceneExplorer.h"
#include <memory>

namespace PaintsNow {
	namespace NsSnowyStream {
		class ResourceManager;
	}
	namespace NsGalaxyWeaver {
		class Service {
		public:
			void Initialize(ISceneExplorer* sceneExp);
			void Deinitialize();

			void Reconnect(const String& port);
			operator bool() const {
				return controller.get() != nullptr;
			}

			Controller* operator -> () const {
				return controller.get();
			}

			NsSnowyStream::ResourceManager& GetResourceManager() const;
			IScript::Request& GetMainRequest() const;
			bool IsConnected() const;

		protected:
			void StatusHandler(IScript::Request& request, bool isServer, ZRemoteProxy::STATUS status, const String& message);

		protected:
			void OnInitializeQuery(IScript::Request& request, IReflectObject& inter, const IScript::Request::Ref& ref);
			void OnCreateWeaverObject(IScript::Request& request, IScript::Request::Ref instance);
			void OnWeaverObjectQuery(IScript::Request& request, IReflectObject& inter, const IScript::Request::Ref& ref);
			void OnCheckVersion(IScript::Request& request, String& mainVersion, String& subVersion, String& buildVersion);

		protected:
			std::unique_ptr<ZRemoteProxy> remoteProxy;
			std::unique_ptr<Controller> controller;
			std::unique_ptr<IScript::Request> mainRequest;
			std::unique_ptr<NsSnowyStream::ResourceManager> resourceManager;
			ISceneExplorer* sceneExp;
		};
	}
}

#endif // __SERVICE_H__
