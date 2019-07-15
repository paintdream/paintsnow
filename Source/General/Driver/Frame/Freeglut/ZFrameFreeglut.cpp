#include "ZFrameFreeglut.h"

#if USE_STATIC_THIRDPARTY_LIBRARIES
#define FREEGLUT_STATIC
#define GLEW_STATIC
#ifdef FREEGLUT_LIB_PRAGMAS
#undef FREEGLUT_LIB_PRAGMAS
#endif
#endif

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <map>
#include <cassert>
using namespace PaintsNow;

static std::map<int, ZFrameFreeglut*> mapWindows;

static int AddKeyModifier(int key);
static int ConvertSpecialKey(int special);
static void ChangeSize(int w, int h);
static void RenderScene();
static void SpecialKeys(int key, int x, int y);
static void SpecialKeysUp(int key, int x, int y);
static void KeyboardFunc(unsigned char cAscii, int x, int y);
static void KeyboardFuncUp(unsigned char cAscii, int x, int y);
static void MouseClickMessage(int button, int state, int x, int y);
static void MouseMoveMessage(int x, int y);
static void MouseWheelMessage(int wheel, int dir, int x, int y);
static void MousePassiveMoveMessage(int x, int y);

static int AddKeyModifier(int key) {
	int mod = glutGetModifiers();

	if ((mod & GLUT_ACTIVE_ALT) == GLUT_ACTIVE_ALT) {
		key |= IFrame::EventKeyboard::KEY_ALT;
	}

	if ((mod & GLUT_ACTIVE_CTRL) == GLUT_ACTIVE_CTRL) {
		key |= IFrame::EventKeyboard::KEY_CTRL;
	}

	if ((mod & GLUT_ACTIVE_SHIFT) == GLUT_ACTIVE_SHIFT) {
		key |= IFrame::EventKeyboard::KEY_SHIFT;
	}

	return key;
}

static int ConvertSpecialKey(int special) {
	int key = special & IFrame::EventKeyboard::KEY_POP;
	switch (special & ~IFrame::EventKeyboard::KEY_POP) {
	case GLUT_KEY_F1:
		key |= IFrame::EventKeyboard::KEY_F1;
		break;
	case GLUT_KEY_F2:
		key |= IFrame::EventKeyboard::KEY_F2;
		break;
	case GLUT_KEY_F3:
		key |= IFrame::EventKeyboard::KEY_F3;
		break;
	case GLUT_KEY_F4:
		key |= IFrame::EventKeyboard::KEY_F4;
		break;
	case GLUT_KEY_F5:
		key |= IFrame::EventKeyboard::KEY_F5;
		break;
	case GLUT_KEY_F6:
		key |= IFrame::EventKeyboard::KEY_F6;
		break;
	case GLUT_KEY_F7:
		key |= IFrame::EventKeyboard::KEY_F7;
		break;
	case GLUT_KEY_F8:
		key |= IFrame::EventKeyboard::KEY_F8;
		break;
	case GLUT_KEY_F9:
		key |= IFrame::EventKeyboard::KEY_F9;
		break;
	case GLUT_KEY_F10:
		key |= IFrame::EventKeyboard::KEY_F10;
		break;
	case GLUT_KEY_F11:
		key |= IFrame::EventKeyboard::KEY_F11;
		break;
	case GLUT_KEY_F12:
		key |= IFrame::EventKeyboard::KEY_F12;
		break;
	case GLUT_KEY_LEFT:
		key |= IFrame::EventKeyboard::KEY_LEFT;
		break;
	case GLUT_KEY_UP:
		key |= IFrame::EventKeyboard::KEY_UP;
		break;
	case GLUT_KEY_RIGHT:
		key |= IFrame::EventKeyboard::KEY_RIGHT;
		break;
	case GLUT_KEY_DOWN:
		key |= IFrame::EventKeyboard::KEY_DOWN;
		break;
	case GLUT_KEY_PAGE_UP:
		key |= IFrame::EventKeyboard::KEY_PAGE_UP;
		break;
	case GLUT_KEY_PAGE_DOWN:
		key |= IFrame::EventKeyboard::KEY_PAGE_DOWN;
		break;
	case GLUT_KEY_HOME:
		key |= IFrame::EventKeyboard::KEY_HOME;
		break;
	case GLUT_KEY_END:
		key |= IFrame::EventKeyboard::KEY_END;
		break;
	case GLUT_KEY_INSERT:
		key |= IFrame::EventKeyboard::KEY_INSERT;
		break;
	case GLUT_KEY_NUM_LOCK:
		key |= IFrame::EventKeyboard::KEY_NUM_LOCK;
		break;
	case GLUT_KEY_BEGIN:
		key |= IFrame::EventKeyboard::KEY_BEGIN;
		break;
	case GLUT_KEY_DELETE:
		key |= IFrame::EventKeyboard::KEY_DELETE;
		break;
	case GLUT_KEY_CTRL_L:
	case GLUT_KEY_CTRL_R:
		key |= IFrame::EventKeyboard::KEY_CTRL;
		break;
	case GLUT_KEY_ALT_L:
	case GLUT_KEY_ALT_R:
		key |= IFrame::EventKeyboard::KEY_ALT;
		break;
	case GLUT_KEY_SHIFT_L:
	case GLUT_KEY_SHIFT_R:
		key |= IFrame::EventKeyboard::KEY_SHIFT;
		break;
		/*
	case GLUT_KEY_CTRL_L:
		key |= IFrame::EventKeyboard::KEY_CTRL;
		break;
	case GLUT_KEY_CTRL_R:
		key |= IFrame::EventKeyboard::KEY_RIGHT_CTRL;
		break;
	case GLUT_KEY_ALT_L:
		key |= IFrame::EventKeyboard::KEY_ALT;
		break;
	case GLUT_KEY_ALT_R:
		key |= IFrame::EventKeyboard::KEY_RIGHT_ALT;
		break;
	case GLUT_KEY_SHIFT_L:
		key |= IFrame::EventKeyboard::KEY_SHIFT;
		break;
	case GLUT_KEY_SHIFT_R:
		key |= IFrame::EventKeyboard::KEY_RIGHT_SHIFT;
		break;
		*/
	}

	return key;
}

static Short2 MakeScreenPos(ZFrameFreeglut* p, int x, int y) {
	return Short2(safe_cast<short>(x), safe_cast<short>(p->GetWindowSize()[1] - y));
}

static int LastButton = GLUT_LEFT_BUTTON;

static void MouseClickMessage(int button, int state, int x, int y) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];
	assert(p != nullptr);

	p->MouseClickMessage(button, state, x, y);
}

void ZFrameFreeglut::MouseClickMessage(int button, int state, int x, int y) {
	LastButton = button;
	if (button == GLUT_LEFT_BUTTON || button == GLUT_RIGHT_BUTTON) {
		OnMouse(IFrame::EventMouse(state == 0, false, button == GLUT_LEFT_BUTTON, false, MakeScreenPos(this, x, y)));
	} else {
		// There is a bug in freeglut_linux . Wheel event is triggered by MouseClickMessage except MouseWhellMessage
		#ifndef _WIN32
		MouseWheelMessage(1, button == 3 ? 1 : -1,  x, y);
		#endif
	}
}

static void MouseMoveMessage(int x, int y) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];
	assert(p != nullptr);
	p->MouseMoveMessage(x, y);
}

void ZFrameFreeglut::MouseMoveMessage(int x, int y) {
	OnMouse(IFrame::EventMouse(true, true, LastButton == GLUT_LEFT_BUTTON, false, MakeScreenPos(this, x, y)));
}

static void MousePassiveMoveMessage(int x, int y) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];
	assert(p != nullptr);

	p->MousePassiveMoveMessage(x, y);
}

void ZFrameFreeglut::MousePassiveMoveMessage(int x, int y) {
	OnMouse(IFrame::EventMouse(false, true, true, false, MakeScreenPos(this, x, y)));
}

static void MouseWheelMessage(int wheel, int dir, int x, int y) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];
	assert(p != nullptr);

	p->MouseWheelMessage(wheel, dir, x, y);
}

void ZFrameFreeglut::MouseWheelMessage(int wheel, int dir, int x, int y) {
	OnMouse(IFrame::EventMouse(dir > 0, false, false, true, MakeScreenPos(this, x, y)));
}

static void SpecialKeys(int key, int x, int y) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];

	assert(p != nullptr);

	p->SpecialKeys(key, x, y);
}

void ZFrameFreeglut::SpecialKeys(int key, int x, int y) {
	key = ConvertSpecialKey(key);
	// only push special keys
	if ((key & IFrame::EventKeyboard::KEY_CTRL) == IFrame::EventKeyboard::KEY_CTRL
		|| (key & IFrame::EventKeyboard::KEY_ALT) == IFrame::EventKeyboard::KEY_ALT
		|| (key & IFrame::EventKeyboard::KEY_SHIFT) == IFrame::EventKeyboard::KEY_SHIFT) {
		OnKeyboard(IFrame::EventKeyboard(key));
	} else {
		OnKeyboard(IFrame::EventKeyboard(key));
	}
}

static void SpecialKeysUp(int key, int x, int y) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];
	assert(p != nullptr);

	p->SpecialKeysUp(key, x, y);
}

void ZFrameFreeglut::SpecialKeysUp(int key, int x, int y) {
	key = ConvertSpecialKey(key) | IFrame::EventKeyboard::KEY_POP;

	// only push special keys
	if ((key & IFrame::EventKeyboard::KEY_CTRL) == IFrame::EventKeyboard::KEY_CTRL
		|| (key & IFrame::EventKeyboard::KEY_ALT) == IFrame::EventKeyboard::KEY_ALT
		|| (key & IFrame::EventKeyboard::KEY_SHIFT) == IFrame::EventKeyboard::KEY_SHIFT) {
		OnKeyboard(IFrame::EventKeyboard(key));
	} else {
		OnKeyboard(IFrame::EventKeyboard(key));
	}
}

static void KeyboardFunc(unsigned char cAscii, int x, int y) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];
	assert(p != nullptr);

	p->KeyboardFunc(cAscii, x, y);
}

void ZFrameFreeglut::KeyboardFunc(unsigned char cAscii, int x, int y) {
	OnKeyboard(IFrame::EventKeyboard(AddKeyModifier((int)cAscii)));
}


void KeyboardFuncUp(unsigned char cAscii, int x, int y) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];
	assert(p != nullptr);

	p->KeyboardFuncUp(cAscii, x, y);
}

void ZFrameFreeglut::KeyboardFuncUp(unsigned char cAscii, int x, int y) {
	OnKeyboard(IFrame::EventKeyboard(AddKeyModifier((int)cAscii)| IFrame::EventKeyboard::KEY_POP));
}

static void ChangeSize(int w, int h) {
	int nWindow = glutGetWindow();
	ZFrameFreeglut* p = mapWindows[nWindow];
	assert(p != nullptr);

	p->ChangeSize(w, h);
}

void ZFrameFreeglut::ChangeSize(int w, int h) {
	OnWindowSize(Int2(w, h)); // Dispatch
}

static void RenderScene() {
	int nWindow = glutGetWindow(); // The same as above
	ZFrameFreeglut* p = mapWindows[nWindow];
	p->OnRender();
}

ZFrameFreeglut::ZFrameFreeglut(const Int2& size, IFrame::Callback* cb) : windowSize(size), mainLoopStarted(false), isRendering(false) {
	SetCallback(cb);
	static bool inited = false;
	if (!inited) {
		inited = true;
		int argc = 1;
		char* argv = (char*)"PaintsNow.Net";
		glutInit(&argc, &argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL);
		glutInitContextVersion(3, 3);
	}

	// Create Window
	glutInitWindowSize(size.x(), size.y());
	int handle = glutCreateWindow("PaintsNow.Net");
	mapWindows[handle] = this;

	// Register callback functions
	glutReshapeFunc(::ChangeSize);
	glutSpecialFunc(::SpecialKeys);
	glutSpecialUpFunc(::SpecialKeysUp);
	glutKeyboardFunc(::KeyboardFunc);
	glutKeyboardUpFunc(::KeyboardFuncUp);
	glutDisplayFunc(::RenderScene);
	glutMouseFunc(::MouseClickMessage);
	glutPassiveMotionFunc(::MousePassiveMoveMessage);
	glutMotionFunc(::MouseMoveMessage);
	glutMouseWheelFunc(::MouseWheelMessage);
}

void ZFrameFreeglut::SetCallback(IFrame::Callback* cb) {
	callback = cb;
}

void ZFrameFreeglut::SetWindowTitle(const String& title) {
	glutSetWindowTitle(title.c_str());
}

void ZFrameFreeglut::OnMouse(const EventMouse& mouse) {
	if (callback != nullptr) {
		callback->OnMouse(mouse);
	}
}

bool ZFrameFreeglut::IsRendering() const {
	return isRendering;
}

void ZFrameFreeglut::OnRender() {
	if (callback != nullptr) {
		// glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		// glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		isRendering = true;
		int err = glGetError(); // clear error on startup
		callback->OnRender();
		glutSwapBuffers();
		isRendering = false;

		if (mainLoopStarted) {
			glutPostRedisplay(); // continue for next frame rendering
		}
	}
}

void ZFrameFreeglut::OnWindowSize(const Int2& newSize) {
	if (callback != nullptr) {
		windowSize = newSize;
		callback->OnWindowSize(newSize);
		glutPostRedisplay();
	}
}

void ZFrameFreeglut::OnKeyboard(const EventKeyboard& keyboard) {
	if (callback != nullptr) {
		callback->OnKeyboard(keyboard);
	}
}

void ZFrameFreeglut::SetWindowSize(const Int2& size) {
	glutReshapeWindow(size.x(), size.y());
	windowSize = size;
}

const Int2& ZFrameFreeglut::GetWindowSize() const {
	return windowSize;
}

void ZFrameFreeglut::EnterMainLoop() {
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	mainLoopStarted = true;
	glutPostRedisplay(); // Start initial display 
	glutMainLoop();
	mainLoopStarted = false;
}

void ZFrameFreeglut::ExitMainLoop() {
	if (mainLoopStarted) {
		mainLoopStarted = false;
		glutLeaveMainLoop();
	}
}

void ZFrameFreeglut::ShowCursor(CURSOR cursor) {
	int type = GLUT_CURSOR_LEFT_ARROW;
	switch (cursor) {
	case IFrame::ARROW:
		type = GLUT_CURSOR_LEFT_ARROW;
		break;
	case IFrame::CROSS:
		type = GLUT_CURSOR_CROSSHAIR;
		break;
	case IFrame::WAIT:
		type = GLUT_CURSOR_WAIT;
		break;
	case IFrame::NONE:
		type = GLUT_CURSOR_NONE;
		break;
	}
	glutSetCursor(type);
}

void ZFrameFreeglut::WarpCursor(const Int2& position) {
	glutWarpPointer(position.x(), position.y());
}
