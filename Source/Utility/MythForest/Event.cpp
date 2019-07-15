#include "Event.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

#if defined(_MSC_VER) && _MSC_VER <= 1200
Event::Event() : eventID(EVENT_PRETICK) {}
#endif

Event::Event(Engine& e, EVENT_ID id, TShared<SharedTiny> sender, TShared<SharedTiny> d, const String& meta) : engine(e), eventID(id), eventMeta(meta), eventSender(sender), eventDetail(d) {}
