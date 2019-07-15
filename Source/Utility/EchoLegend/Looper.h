#ifndef __LOOPER_H__
#define __LOOPER_H__

#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/INetwork.h"
#include "../../Core/System/Kernel.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	namespace NsEchoLegend {
		class Looper : public TReflected<Looper, WarpTiny> {
		public:
			Looper(NsBridgeSunset::BridgeSunset& bridgeSunset, INetwork& network);
			virtual ~Looper();
			virtual bool Activate() = 0;
			virtual void Deactivate() = 0;
			void AsyncActivate(IScript::Request& request);
			static String EventToString(INetwork::EVENT event);

		protected:
			bool ActivateRoutine(IThread::Thread* thread, size_t);

		protected:
			INetwork& network;
			NsBridgeSunset::BridgeSunset& bridgeSunset;
		};
	}
}

#endif // __LOOPER_H__