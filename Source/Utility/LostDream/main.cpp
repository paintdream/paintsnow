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
#if defined(_WIN32) || defined(WIN32)
	::CoInitialize(nullptr);
#endif

	LostDream lostDream;
	lostDream.RegisterQualifier(TFactory<Memory, LostDream::Qualifier>(), 1);
	lostDream.RegisterQualifier(TFactory<Serialization, LostDream::Qualifier>(), 1);
	lostDream.RegisterQualifier(TFactory<RandomQuery, LostDream::Qualifier>(), 12);
	lostDream.RegisterQualifier(TFactory<RPC, LostDream::Qualifier>(), 1);
	lostDream.RegisterQualifier(TFactory<Annotation, LostDream::Qualifier>(), 1);

	lostDream.RunQualifiers(true, 0, 4);
	
#if defined(_WIN32) || defined(WIN32)
	::CoUninitialize();
#endif
	return 0;
}

