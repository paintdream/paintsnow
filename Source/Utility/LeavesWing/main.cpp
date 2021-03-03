#include "../LeavesFlute/LeavesFlute.h"
#include "../LeavesFlute/Platform.h"
#ifdef _WIN32
#include "../../General/Driver/Debugger/MiniDump/ZDebuggerWin.h"
#include "Helpers/Win32/ServiceWin32.h"
#include <shlwapi.h>
// #include <vld.h>
#endif

#include "../LeavesFlute/Loader.h"
#include <ctime>

#include "Helpers/ImGui/LeavesImGui.h"

#if USE_LEAVES_IMGUI
#include "Helpers/ImGui/System.h"
#include "Helpers/ImGui/Repository.h"
#include "Helpers/ImGui/Script.h"
#include "Helpers/ImGui/IModule.h"
#endif

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
	// Register/unregister service?
#ifdef _WIN32
	if (argc == 2) {
		if (strcmp(argv[1], "/install") == 0) {
			if (ServiceWin32::GetInstance().InstallService()) {
				printf("Service installation complete!\n");
				return 0;
			} else {
				printf("Service installation error!\n");
				return -1;
			}
		} else if (strcmp(argv[1], "/uninstall") == 0) {
			if (ServiceWin32::GetInstance().DeleteService()) {
				printf("Service uninstallation complete!\n");
				return 0;
			} else {
				printf("Service uninstallation error!\n");
				return -1;
			}
		}
		else if (strcmp(argv[1], "/worker") == 0)
		{
			ServiceWin32::GetInstance().RunServiceWorker(argc, argv);
			return 0;
		}
	}

	if (argc == 1 && ServiceWin32::InServiceContext()) {
		ServiceWin32::GetInstance().RunServiceMaster(argc, argv);
		return 0;
	}
#endif

	RegisterDump();

	CmdLine cmdLine;

	// support for drag'n'drop startup profile
#ifdef _WIN32
	if (argc == 2 && argv[1][0] != '-') {
		char target[MAX_PATH * 2] = "";
		char current[MAX_PATH * 2] = "";
		::GetCurrentDirectoryA(MAX_PATH * 2, current);
		String mount;
		if (::PathRelativePathToA(target, current, FILE_ATTRIBUTE_DIRECTORY, argv[1], FILE_ATTRIBUTE_NORMAL)) {
			mount = String("--Mount=") + target;
		} else {
			mount = String("--Mount=") + argv[1];
		}

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

	printf("LeavesWing %s\nPaintDream (paintdream@paintdream.com) (C) 2014-2021\nBased on PaintsNow [https://github.com/paintdream/paintsnow]\n", PAINTSNOW_VERSION_MINOR);

	Loader loader;
#if USE_LEAVES_IMGUI
	std::map<String, CmdLine::Option>::const_iterator it = cmdLine.GetFactoryMap().find("IFrame");
	if (it != cmdLine.GetFactoryMap().end() && it->second.name == "ZFrameGLFWImGui") {
		System system;
		IModule visualizer;
		Repository repository;
		Script script;

		LeavesImGui leavesImGui(loader.leavesFlute);
		TWrapper<IFrame*> frameFactory = WrapFactory(UniqueType<ZFrameGLFWImGui>(), std::ref(leavesImGui));

		loader.config.RegisterFactory("IFrame", "ZFrameGLFWImGui", frameFactory);
		leavesImGui.AddWidget(&system);
		leavesImGui.AddWidget(&repository);
		leavesImGui.AddWidget(&visualizer);
		leavesImGui.AddWidget(&script);
		loader.Run(cmdLine);
	} else {
		loader.Run(cmdLine);
	}
#else
	loader.Run(cmdLine);
#endif

#ifdef _WIN32
	OutputDebugStringA("LeavesWing exited without any errors.\n");
#endif
	return 0;
}
