// Provider.h
// By PaintDream (paintdream@paintdream.com)
// 2018-2-9
//

#ifndef __PROVIDER_H__
#define __PROVIDER_H__

#define _WIN32_WINNT_WIN10_RS1 0
#define _WIN32_WINNT_WIN10_RS2 0
#define _WIN32_WINNT_WIN10_RS3 0
#define _WIN32_WINNT_WIN10_TH2 0
#define _APISET_RTLSUPPORT_VER 0
#define _APISET_INTERLOCKED_VER 0
#define _WIN32_WINNT_WINTHRESHOLD 0
#define _APISET_SECURITYBASE_VER 0
#define NTDDI_WIN7SP1 0

#pragma warning(disable:4706)
#define PAINTSNOW_NO_PREIMPORT_LIBS
#include "../../../../../Source/Utility/GalaxyWeaver/Controller.h"
#include "../../../../../Source/Utility/GalaxyWeaver/Weaver.h"

namespace PaintsNow {
	namespace NsGalaxyWeaver {
		class Provider : public Controller {
		public:
			Provider(IThread& threadApi, ITunnel& tunnel, const String& entry);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void Destroy(IScript::Request& request);
		};

		class ProviderFactory : public TFactoryBase<IScript::Object> {
		public:
			ProviderFactory(IThread& threadApi, ITunnel& t);
			IScript::Object* CreateObject(const String& entry) const;
			IThread& thread;
			ITunnel& tunnel;
		};
	}
}

#endif // __PROVIDER_H__