#include "../LeavesFlute/LeavesFlute.h"
#ifdef _WIN32
#include "../../General/Driver/Debugger/MiniDump/ZDebuggerWin.h"
#include <shlwapi.h>
// #include <vld.h>
#endif

#include "../LeavesFlute/Loader.h"
#include <ctime>

using namespace PaintsNow;

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

	// support for drag'n'drop startup profile
#ifdef _WIN32
	if (argc == 2 && argv[1][0] != '-') {
		char target[MAX_PATH * 2] = "";
		char current[MAX_PATH * 2] = "";
		::GetCurrentDirectoryA(MAX_PATH * 2, current);
		::PathRelativePathToA(target, current, FILE_ATTRIBUTE_DIRECTORY, argv[1], FILE_ATTRIBUTE_NORMAL);
		String mount = String("--Mount=") + target;

		char* dragArgs[] = {
			argv[0],
			"--Graphic=true",
			const_cast<char*>(mount.c_str())
		};
		
		cmdLine.Process(3, dragArgs);
	} else {
		cmdLine.Process(argc, argv);
	}
#else
	cmdLine.Process(argc, argv);
#endif

	Loader loader;
	loader.Load(cmdLine);

	return 0;
}
