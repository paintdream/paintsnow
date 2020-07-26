#include "ZFrameGLFW.h"

#if USE_STATIC_THIRDPARTY_LIBRARIES
#define GLFW_STATIC
#define GLEW_STATIC
#ifdef GLFW_LIB_PRAGMAS
#undef GLFW_LIB_PRAGMAS
#endif
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
using namespace PaintsNow;

static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

void ZFrameGLFW::mouse_button_callback(int button, int action, int mods) {
	double cursorX, cursorY;
	glfwGetCursorPos(window, &cursorX, &cursorY);
	IFrame::EventMouse event(lastdown = (action == GLFW_PRESS), false, lastbutton = (button == GLFW_MOUSE_BUTTON_LEFT), false, Short2((short)cursorX, (short)cursorY));

	OnMouse(event);
}

void ZFrameGLFW::cursor_position_callback(double x, double y) {
	IFrame::EventMouse event(lastdown, true, lastbutton, false, Short2((short)x, (short)y));

	OnMouse(event);
}

void ZFrameGLFW::scroll_callback(double x, double y) {
	IFrame::EventMouse event(y > 0, false, false, true, Short2(0, 0));

	OnMouse(event);
}

void ZFrameGLFW::framebuffer_size_callback(int width, int height) {
	OnWindowSize(IFrame::EventSize(Int2(width, height)));
}

void ZFrameGLFW::key_callback(int key, int scancode, int action, int mods) {
	
	IFrame::EventKeyboard event(key > 0xff ? 0 : key);

	if (action == GLFW_RELEASE) {
		event.keyCode |= IFrame::EventKeyboard::KEY_POP;
	}

	if (key > 0xff && key <= GLFW_KEY_LAST) {
		event.keyCode |= IFrame::EventKeyboard::KEY_ESCAPE + key - GLFW_KEY_ESCAPE;
	}

	if (key & GLFW_MOD_SHIFT) {
		event.keyCode |= IFrame::EventKeyboard::KEY_SHIFT;
	}

	if (key & GLFW_MOD_CONTROL) {
		event.keyCode |= IFrame::EventKeyboard::KEY_CTRL;
	}

	if (key & GLFW_MOD_ALT) {
		event.keyCode |= IFrame::EventKeyboard::KEY_ALT;
	}

	OnKeyboard(event);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->mouse_button_callback(button, action, mods);
}

void cursor_position_callback(GLFWwindow* window, double x, double y) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->cursor_position_callback(x, y);
}

void scroll_callback(GLFWwindow* window, double x, double y) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->scroll_callback(x, y);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->framebuffer_size_callback(width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	ZFrameGLFW* frame = static_cast<ZFrameGLFW*>(glfwGetWindowUserPointer(window));
	frame->key_callback(key, scancode, action, mods);
}

ZFrameGLFW::ZFrameGLFW(const Int2& size, IFrame::Callback* cb) : windowSize(size), mainLoopStarted(false), isRendering(false), lastdown(false), lastbutton(false) {
	SetCallback(cb);
	glfwSetErrorCallback(error_callback);

	glfwInit();
	window = glfwCreateWindow(size.x(), size.y(), "PaintsNow.Net", NULL, NULL);

	glfwSetWindowUserPointer(window, this);

	OnWindowSize(size);
	init();

	glfwSetKeyCallback(window, ::key_callback);
	glfwSetFramebufferSizeCallback(window, ::framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, ::mouse_button_callback);
	glfwSetCursorPosCallback(window, ::cursor_position_callback);
	glfwSetScrollCallback(window, ::scroll_callback);
	glfwMakeContextCurrent(window);
}

void ZFrameGLFW::init() {}

ZFrameGLFW::~ZFrameGLFW() {
	glfwTerminate();
}

void ZFrameGLFW::SetCallback(IFrame::Callback* cb) {
	callback = cb;
	if (callback != nullptr) {
		callback->OnWindowSize(windowSize);
	}
}

void ZFrameGLFW::SetWindowTitle(const String& title) {
	glfwSetWindowTitle(window, title.c_str());
}

void ZFrameGLFW::OnMouse(const EventMouse& mouse) {
	if (callback != nullptr) {
		EventMouse m = mouse;
		m.position.y() = windowSize.y() - mouse.position.y();
		callback->OnMouse(m);
	}
}

bool ZFrameGLFW::IsRendering() const {
	return isRendering;
}

void ZFrameGLFW::OnWindowSize(const EventSize& newSize) {
	windowSize = newSize.size;
	if (callback != nullptr) {
		callback->OnWindowSize(newSize);
	}
}

void ZFrameGLFW::OnKeyboard(const EventKeyboard& keyboard) {
	if (callback != nullptr) {
		callback->OnKeyboard(keyboard);
	}
}

void ZFrameGLFW::SetWindowSize(const Int2& size) {
	glfwSetWindowSize(window, size.x(), size.y());
	windowSize = size;
}

const Int2& ZFrameGLFW::GetWindowSize() const {
	return windowSize;
}

void ZFrameGLFW::render() {}

void ZFrameGLFW::EnterMainLoop() {
	mainLoopStarted = true;
	while (!glfwWindowShouldClose(window) && mainLoopStarted) 		{
		if (callback != nullptr) {
			// glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			// glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			isRendering = true;
			callback->OnRender();

			render();
			glfwSwapBuffers(window);
			isRendering = false;
		}

		glfwPollEvents();
	}

	mainLoopStarted = false;
}

void ZFrameGLFW::ExitMainLoop() {
	mainLoopStarted = false;
}

void ZFrameGLFW::ShowCursor(CURSOR cursor) {
	int type = GLFW_CURSOR_NORMAL;
	switch (cursor) {
	case IFrame::ARROW:
		type = GLFW_CURSOR_NORMAL;
		break;
	case IFrame::CROSS:
		type = GLFW_CURSOR_NORMAL;
		break;
	case IFrame::WAIT:
		type = GLFW_CURSOR_DISABLED;
		break;
	case IFrame::NONE:
		type = GLFW_CURSOR_HIDDEN;
		break;
	}

	glfwSetInputMode(window, GLFW_CURSOR, type);
}

void ZFrameGLFW::WarpCursor(const Int2& position) {
	glfwSetCursorPos(window, position.x(), position.y());
}

// GLFW provides release version libs ONLY!?
#if defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER > 1200
extern "C" void _except_handler4_common() {}
#endif
