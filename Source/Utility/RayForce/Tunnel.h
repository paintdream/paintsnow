// Tunnel.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-4
//

#ifndef __TUNNEL_H__
#define __TUNNEL_H__

#include "../../Core/Interface/IScript.h"
#include "../../Core/System/Kernel.h"
#include "Proxy.h"

namespace PaintsNow {
	namespace NsRayForce {
		class Bridge;
		class Tunnel : public TReflected<Tunnel, WarpTiny> {
		public:
			Tunnel(Bridge* bridge, IReflectObject* host);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual ~Tunnel();
			void ForwardCall(const TProxy<>* p, IScript::Request& request);
			Proxy& NewProxy(const TProxy<>* p);
			IReflectObject* GetHost() const;
			void Dump(IScript::Request& request);

		private:
			Bridge* bridge;
			IReflectObject* host;
			std::list<Proxy> proxy;
		};
	}
}


#endif // __TUNNEL_H__