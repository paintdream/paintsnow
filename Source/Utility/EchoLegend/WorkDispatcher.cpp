#include "WorkDispatcher.h"

using namespace PaintsNow;

WorkDispatcher::WorkDispatcher(BridgeSunset& bridgeSunset, INetwork& network, ITunnel::Dispatcher* disp) : BaseClass(std::ref(bridgeSunset), std::ref(network)), dispatcher(disp) {}

ITunnel::Dispatcher* WorkDispatcher::GetDispatcher() const {
	return dispatcher;
}

bool WorkDispatcher::Activate() {
	return network.ActivateDispatcher(dispatcher);
}
void WorkDispatcher::Deactivate() {
	network.DeactivateDispatcher(dispatcher);
}