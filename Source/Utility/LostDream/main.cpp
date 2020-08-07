// LostDream Test Entry
#include "../../Core/PaintsNow.h"
#if defined(_WIN32) || defined(WIN32)
#include <comdef.h>
// #include <vld.h>
#endif
#include "../LeavesFlute/LeavesFlute.h"

#if !defined(CMAKE_PAINTSNOW) || ADD_THREAD_PTHREAD
#include "../../Core/Driver/Thread/Pthread/ZThreadPthread.h"
#endif

#include "LostDream.h"
#include "Spatial/Spatial.h"
#include "Reflection/Reflection.h"
#include "../LeavesFlute/Platform.h"

using namespace PaintsNow;
using namespace PaintsNow::NsLostDream;

#if defined(_MSC_VER) && _MSC_VER <= 1200 && _DEBUG
extern "C" int _CrtDbgReport() {
	return 0;
}
#endif

int main(void) {
	LostDream lostDream;
	lostDream.RegisterQualifier(WrapFactory(UniqueType<Memory>()), 1);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<Serialization>()), 1);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<RandomQuery>()), 12);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<RPC>()), 1);
	lostDream.RegisterQualifier(WrapFactory(UniqueType<Annotation>()), 1);

	lostDream.RunQualifiers(true, 0, 4);
	return 0;
}

