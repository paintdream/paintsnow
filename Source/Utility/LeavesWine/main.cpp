#include "../../Core/System/Kernel.h"
#include "../../Core/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../General/Driver/Frame/Freeglut/ZFrameFreeglut.h"
#include "../LeavesFlute/Platform.h"
#include "../LeavesFlute/Loader.h"
#include "LeavesWine.h"
#include "System.h"
#include "Repository.h"
#include "Script.h"
#include "Module.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glut.h"
#include "ImGui/imgui_impl_opengl3.h"

#if USE_STATIC_THIRDPARTY_LIBRARIES
#define FREEGLUT_STATIC
#define GLEW_STATIC
#ifdef FREEGLUT_LIB_PRAGMAS
#undef FREEGLUT_LIB_PRAGMAS
#endif
#endif

#include <GL/glew.h>
#include <GL/freeglut.h>

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesFlute;
using namespace PaintsNow::NsLeavesWine;

/*
IMGUI_IMPL_API void     ImGui_ImplGLUT_ReshapeFunc(int w, int h);                           // ~ ResizeFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MotionFunc(int x, int y);                            // ~ MouseMoveFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MouseFunc(int button, int state, int x, int y);      // ~ MouseButtonFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MouseWheelFunc(int button, int dir, int x, int y);   // ~ MouseWheelFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_KeyboardFunc(unsigned char c, int x, int y);         // ~ CharPressedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_KeyboardUpFunc(unsigned char c, int x, int y);       // ~ CharReleasedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_SpecialFunc(int key, int x, int y);                  // ~ KeyPressedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_SpecialUpFunc(int key, int x, int y);                // ~ KeyReleasedFunc
*/


class ZFrameFreeglutForImGui : public ZFrameFreeglut {
public:
	ZFrameFreeglutForImGui(LeavesWine& wine, const Int2& size = Int2(800, 600), Callback* callback = nullptr) : ZFrameFreeglut(size, callback), leavesWine(wine) {}

	virtual void ChangeSize(int w, int h) {
		ImGui_ImplGLUT_ReshapeFunc(w, h);
		ZFrameFreeglut::ChangeSize(w, h);
	}

	virtual void SpecialKeys(int key, int x, int y) {
		ImGui_ImplGLUT_SpecialFunc(key, x, y);
		if (ImGui::GetIO().WantCaptureKeyboard) return;
		ZFrameFreeglut::SpecialKeys(key, x, y);
	}

	virtual void SpecialKeysUp(int key, int x, int y) {
		ImGui_ImplGLUT_SpecialUpFunc(key, x, y);
		if (ImGui::GetIO().WantCaptureKeyboard) return;
		ZFrameFreeglut::SpecialKeysUp(key, x, y);
	}

	virtual void KeyboardFunc(unsigned char cAscii, int x, int y) {
		ImGui_ImplGLUT_KeyboardFunc(cAscii, x, y);
		if (ImGui::GetIO().WantCaptureKeyboard) return;
		ZFrameFreeglut::KeyboardFunc(cAscii, x, y);
	}

	virtual void KeyboardFuncUp(unsigned char cAscii, int x, int y) {
		ImGui_ImplGLUT_KeyboardUpFunc(cAscii, x, y);
		if (ImGui::GetIO().WantCaptureKeyboard) return;
		ZFrameFreeglut::KeyboardFuncUp(cAscii, x, y);
	}

	virtual void MouseClickMessage(int button, int state, int x, int y) {
		ImGui_ImplGLUT_MouseFunc(button, state, x, y);
		if (ImGui::GetIO().WantCaptureMouse) return;
		ZFrameFreeglut::MouseClickMessage(button, state, x, y);
	}

	virtual void MouseMoveMessage(int x, int y) {
		ImGui_ImplGLUT_MotionFunc(x, y);
		if (ImGui::GetIO().WantCaptureMouse) return;
		ZFrameFreeglut::MouseMoveMessage(x, y);
	}

	virtual void MouseWheelMessage(int wheel, int dir, int x, int y) {
		ImGui_ImplGLUT_MouseWheelFunc(wheel, dir, x, y);
		if (ImGui::GetIO().WantCaptureMouse) return;
		ZFrameFreeglut::MouseWheelMessage(wheel, dir, x, y);
	}

	virtual void MousePassiveMoveMessage(int x, int y) {
		ImGui_ImplGLUT_MotionFunc(x, y);
		if (ImGui::GetIO().WantCaptureMouse) return;
		ZFrameFreeglut::MousePassiveMoveMessage(x, y);
	}

	virtual void OnRender() {
		if (callback != nullptr) {
			// glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			// glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			isRendering = true;
			int err = glGetError(); // clear error on startup
			callback->OnRender();

			glUseProgram(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGLUT_NewFrame();

			static bool showAll = true;
			ImGuiIO& io = ImGui::GetIO();
			if (ImGui::IsKeyPressed('`')) {
				showAll = !showAll;
			}

			if (showAll) {
				ImGui::ShowDemoWindow();
				leavesWine.TickRender();
			}

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glutSwapBuffers();
			isRendering = false;

			if (mainLoopStarted) {
				glutPostRedisplay(); // continue for next frame rendering
			}
		}
	}

	LeavesWine& leavesWine;
};

class FrameImGuiFactory : public TFactoryBase<IFrame> {
public:
	FrameImGuiFactory(LeavesWine& wine) : leavesWine(wine), TFactoryBase<IFrame>(Wrap(this, &FrameImGuiFactory::CreateInstance)) {}
	IFrame* CreateInstance(const String& info = "") const {
		return new ZFrameFreeglutForImGui(leavesWine);
	}

	LeavesWine& leavesWine;
};

int main(int argc, char* argv[]) {
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGui_ImplGLUT_Init();
	ImGui_ImplOpenGL3_Init();

	ImGui::StyleColorsClassic();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.Fonts->AddFontDefault();
	io.FontAllowUserScaling = true;

	static_assert(std::is_same<std::remove_pointer<TShared<SharedTiny>>::type, SharedTiny>::value, "Remove pointer");

	CmdLine cmdLine;
	cmdLine.Process(argc, argv);
	const char* hook[] = { nullptr, "--IFrame=ZFrameFreeglutForImGui" };
	cmdLine.Process(2, const_cast<char**>(hook));

	Loader loader;
	{
		System system;
		Module visualizer;
		Repository repository;
		Script script;

		LeavesWine leavesWine(loader.leavesFlute);
		const FrameImGuiFactory sframeFactory(leavesWine);

		loader.config.RegisterFactory("IFrame", "ZFrameFreeglutForImGui", sframeFactory);
		leavesWine.AddWidget(&system);
		leavesWine.AddWidget(&repository);
		leavesWine.AddWidget(&visualizer);
		leavesWine.AddWidget(&script);

		loader.Load(cmdLine);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGLUT_Shutdown();
	ImGui::DestroyContext();

	return 0;
}
