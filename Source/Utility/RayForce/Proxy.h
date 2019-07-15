// Proxy.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-4
//

#ifndef __PROXY_H__
#define __PROXY_H__

#include "../../Core/Interface/IScript.h"
#include "../../Core/Template/TProxy.h"

namespace PaintsNow {
	namespace NsRayForce {
		class Tunnel;
		class Proxy {
		public:
			Proxy(Tunnel* hostTunnel = nullptr, const TProxy<>* routine = nullptr);
			void OnCall(IScript::Request& request);

		private:
			Tunnel* hostTunnel;
			const TProxy<>* routine;
		};
	}
}

#endif // __PROXY_H__