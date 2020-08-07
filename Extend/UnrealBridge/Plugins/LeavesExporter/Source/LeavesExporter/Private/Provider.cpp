#include "LeavesExporterPCH.h"
#include "Provider.h"

#ifdef UE_BUILD_DEBUG
#pragma comment(lib, "pthreadD.lib")
#else
#pragma comment(lib, "pthread.lib")
#endif

namespace PaintsNow {
	namespace NsGalaxyWeaver {

		Provider::Provider(IThread& threadApi, ITunnel& tunnel, const String& entry) : Controller(threadApi, tunnel, entry) {}

		TObject<IReflect>& Provider::operator () (IReflect& reflect) {
			ReflectClass(Provider);
			if (reflect.IsReflectMethod()) {
			}

			return *this;
		}

		void Provider::Destroy(IScript::Request& request) {}

		ProviderFactory::ProviderFactory(IThread& threadApi, ITunnel& t) : TWrapper<IScript::Object*, const String&>(Wrap(this, &ProviderFactory::CreateObject)), thread(threadApi), tunnel(t) {}

		IScript::Object* ProviderFactory::CreateObject(const String& entry) const {
			return new Provider(thread, tunnel, entry);
		}
	}
}