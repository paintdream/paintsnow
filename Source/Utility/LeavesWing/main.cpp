#include "../LeavesFlute/LeavesFlute.h"
#ifdef _WIN32
#include "../../General/Driver/Debugger/MiniDump/ZDebuggerWin.h"
// #include <vld.h>
#endif

#include "../LeavesFlute/Loader.h"
#include <ctime>

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesFlute;

static bool DumpHandler() {
	// always write minidump file
	return true;
}

static void RegisterDump() {
#if defined(_WIN32) || defined(WIN32)
	ZDebuggerWin dumper;
	time_t t;
	time(&t);
	tm* x = localtime(&t);
	char fileName[256];
	sprintf(fileName, "LeavesWingCrashLog_%04d_%02d_%02d_%02d_%02d_%02d.dmp", x->tm_year, x->tm_mon, x->tm_mday, x->tm_hour, x->tm_min, x->tm_sec);
	dumper.SetDumpHandler(fileName, &DumpHandler);
#endif
}

int main(int argc, char* argv[]) {
	RegisterDump();

	CmdLine cmdLine;
	cmdLine.Process(argc, argv);

	Loader loader;
	loader.Load(cmdLine);

	return 0;
}
