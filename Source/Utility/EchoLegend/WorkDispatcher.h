// WorkDispatcher.h
// By PaintDream (paintdream@paintdream.com)
// 2018-2-1
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/INetwork.h"
#include "Looper.h"

namespace PaintsNow {
	class WorkDispatcher : public TReflected<WorkDispatcher, Looper> {
	public:
		WorkDispatcher(BridgeSunset& bridgeSunset, INetwork& network, ITunnel::Dispatcher* disp);
		virtual bool Activate();
		virtual void Deactivate();
		ITunnel::Dispatcher* GetDispatcher() const;

	protected:
		ITunnel::Dispatcher* dispatcher;
	};
}

