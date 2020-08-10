#include "../../Core/System/Kernel.h"
#include "../../Core/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../General/Driver/Frame/GLFW/ZFrameGLFW.h"
#include "../LeavesFlute/Platform.h"
#include "../LeavesFlute/Loader.h"
#include "LeavesWine.h"
#include "System.h"
#include "Repository.h"
#include "Script.h"
#include "Module.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

#if USE_STATIC_THIRDPARTY_LIBRARIES
#define GLFW_STATIC
#define GLEW_STATIC
#ifdef GLFW_LIB_PRAGMAS
#undef GLFW_LIB_PRAGMAS
#endif
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesFlute;
using namespace PaintsNow::NsLeavesWine;

/*
IMGUI_IMPL_API bool	 ImGui_ImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API bool	 ImGui_ImplGlfw_InitForVulkan(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API void	 ImGui_ImplGlfw_Shutdown();
IMGUI_IMPL_API void	 ImGui_ImplGlfw_NewFrame();

// GLFW callbacks
// - When calling Init with 'install_callbacks=true': GLFW callbacks will be installed for you. They will call user's previously installed callbacks, if any.
// - When calling Init with 'install_callbacks=false': GLFW callbacks won't be installed. You will need to call those function yourself from your own GLFW callbacks.
IMGUI_IMPL_API void	 ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
IMGUI_IMPL_API void	 ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_IMPL_API void	 ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
IMGUI_IMPL_API void	 ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c);

*/

class ZFrameGLFWForImGui : public ZFrameGLFW {
public:
	ZFrameGLFWForImGui(LeavesWine& wine, const Int2& size = Int2(1450, 800), Callback* callback = nullptr) : ZFrameGLFW(nullptr, false, size, callback), leavesWine(wine), init(false) {}

	virtual void OnMouseButtonCallback(int button, int action, int mods) override {
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
		if (ImGui::GetIO().WantCaptureMouse) return;
		ZFrameGLFW::OnMouseButtonCallback(button, action, mods);
	}

	virtual void OnScrollCallback(double x, double y) override {
		ImGui_ImplGlfw_ScrollCallback(window, x, y);
		if (ImGui::GetIO().WantCaptureMouse) return;
		ZFrameGLFW::OnScrollCallback(x, y);
	}

	virtual void OnKeyboardCallback(int key, int scancode, int action, int mods) override {
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
		if (ImGui::GetIO().WantCaptureKeyboard) return;
		ZFrameGLFW::OnKeyboardCallback(key, scancode, action, mods);
	}

	virtual void OnCustomRender() override {
		if (!init) {
			IMGUI_CHECKVERSION();

			ImGui::CreateContext();
			ImGui_ImplGlfw_InitForOpenGL(window, false);
			ImGui_ImplOpenGL3_Init();

			ImGui::StyleColorsClassic();

			ImGuiIO& io = ImGui::GetIO();
			io.IniFilename = NULL;
			io.Fonts->AddFontDefault();
			io.FontAllowUserScaling = true;

			init = true;
		}

		glUseProgram(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

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
	}

	LeavesWine& leavesWine;
	bool init;
};

int main(int argc, char* argv[]) {
	CmdLine cmdLine;
	cmdLine.Process(argc, argv);
	const char* hook[] = { nullptr, "--IFrame=ZFrameGLFWForImGui" };
	cmdLine.Process(2, const_cast<char**>(hook));

	Loader loader;
	{
		System system;
		Module visualizer;
		Repository repository;
		Script script;

		LeavesWine leavesWine(loader.leavesFlute);
		TWrapper<IFrame*> frameFactory = WrapFactory(UniqueType<ZFrameGLFWForImGui>(), std::ref(leavesWine));

		loader.config.RegisterFactory("IFrame", "ZFrameGLFWForImGui", frameFactory);
		leavesWine.AddWidget(&system);
		leavesWine.AddWidget(&repository);
		leavesWine.AddWidget(&visualizer);
		leavesWine.AddWidget(&script);

		loader.Load(cmdLine);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	return 0;
}
