#define PAINTSNOW_DLL
#define BUILDING_DLL
#include "../LeavesFlute/Platform.h"
#include "../LeavesFlute/Loader.h"
#pragma comment(lib, "Opengl32.lib")

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesFlute;

extern "C" DLL_PUBLIC void* LeavesCreate() {
	return new Loader();
}

extern "C" DLL_PUBLIC bool LeavesRegisterFactory(void* instance, const char* key, const char* name, const void* value) {
	Loader* loader = reinterpret_cast<Loader*>(instance);
	if (loader != nullptr) {
		const TWrapper<void*, const String&>* wrapper = reinterpret_cast<const TWrapper<void*, const String&>*>(value);
		loader->config.RegisterFactory(key, name, wrapper);
		return true;
	} else {
		return false;
	}
}

extern "C" DLL_PUBLIC bool LeavesInitialize(void* instance, int argc, char* argv[]) {
	Loader* loader = reinterpret_cast<Loader*>(instance);
	if (loader != nullptr) {
		CmdLine cmdLine;
		cmdLine.Process(argc, argv);
		loader->Load(cmdLine);
		return true;
	} else {
		return false;
	}
}

extern "C" DLL_PUBLIC bool LeavesUninitialize(void* instance) {
	Loader* loader = reinterpret_cast<Loader*>(instance);
	loader->leavesFlute->GetInterfaces().frame.ExitMainLoop();

	return true;
}

extern "C" DLL_PUBLIC bool LeavesDelete(void* instance) {
	Loader* loader = reinterpret_cast<Loader*>(instance);
	if (loader != nullptr) {
		delete loader;
		return true;
	} else {
		return false;
	}
}

static void* InternalQueryBuiltinFactory(const char* key, const char* name);

extern "C" DLL_PUBLIC void* LeavesQueryBuiltinFactory(const char* key, const char* name) {
	return InternalQueryBuiltinFactory(key, name);
}

#ifdef _WIN32

static IFrame::EventKeyboard ConvertKeyMessage(UINT nChar, UINT nFlags, int extraMask) {
	int key = 0;
	if (nChar >= VK_F1 && nChar <= VK_F12) {
		key += (nChar - VK_F1) + IFrame::EventKeyboard::KEY_F1;
	} else {
		switch (nChar) {
			case VK_PRIOR:
				key += IFrame::EventKeyboard::KEY_PAGE_UP;
				break;
			case VK_NEXT:
				key += IFrame::EventKeyboard::KEY_PAGE_DOWN;
				break;
			case VK_END:
				key += IFrame::EventKeyboard::KEY_END;
				break;
			case VK_HOME:
				key += IFrame::EventKeyboard::KEY_HOME;
				break;
			case VK_LEFT:
				key += IFrame::EventKeyboard::KEY_LEFT;
				break;
			case VK_UP:
				key += IFrame::EventKeyboard::KEY_UP;
				break;
			case VK_RIGHT:
				key += IFrame::EventKeyboard::KEY_RIGHT;
				break;
			case VK_DOWN:
				key += IFrame::EventKeyboard::KEY_DOWN;
				break;
			case VK_INSERT:
				key += IFrame::EventKeyboard::KEY_INSERT;
				break;
			case VK_DELETE:
				key += IFrame::EventKeyboard::KEY_DELETE;
				break;
			case VK_NUMLOCK:
				key += IFrame::EventKeyboard::KEY_NUM_LOCK;
				break;
			default:
				BYTE state[256];
				WORD code[2];

				if (GetKeyboardState(state)) {
					if (ToAscii(nChar, 0, state, code, 0) == 1)
						nChar = code[0] & 0xFF;
				}
				break;
		}
	}

	if (key == 0)
		key = nChar;

	if (::GetKeyState(VK_MENU) < 0)
		key |= IFrame::EventKeyboard::KEY_ALT;
	if (::GetKeyState(VK_CONTROL) < 0)
		key |= IFrame::EventKeyboard::KEY_CTRL;
	if (::GetKeyState(VK_SHIFT) < 0)
		key |= IFrame::EventKeyboard::KEY_SHIFT;

	if (nChar >= 'A' && nChar <= 'Z' && ((key & IFrame::EventKeyboard::KEY_SHIFT) == IFrame::EventKeyboard::KEY_SHIFT))
		key += 'a' - 'A';

	return IFrame::EventKeyboard(key | extraMask);
}

LRESULT CALLBACK HookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class ZFrameWin32AttachWindow : public IFrame {
public:
	ZFrameWin32AttachWindow(const String& param) : hdc(NULL), originalProc(nullptr), isRendering(false) {
		// parse hWnd from param string
		void* p = nullptr;
		sscanf(param.c_str(), "%p", &p);
		window = reinterpret_cast<HWND>(p);

		mainLoopEvent = ::CreateEventW(NULL, TRUE, FALSE, NULL);
	}

	virtual ~ZFrameWin32AttachWindow() {
		::CloseHandle(mainLoopEvent);
	}

	virtual void SetCallback(Callback* cb) {
		callback = cb;
	}

	virtual const Int2& GetWindowSize() const { return windowSize; }

	virtual void SetWindowSize(const Int2& size) {
		windowSize = size;
		::SetWindowPos(window, HWND_TOP, 0, 0, size.x(), size.y(), SWP_NOMOVE);
	}

	virtual void SetWindowTitle(const String& title) {
		::SetWindowTextW(window, (LPCWSTR)Utf8ToSystem(title).c_str());
	}

	virtual void ShowCursor(CURSOR cursor) {
		if (cursor == NONE) {
			::ShowCursor(FALSE);
		} else {
			::ShowCursor(TRUE);
			LPCSTR id = IDC_ARROW;

			switch (cursor) {
				case ARROW:
					id = IDC_ARROW;
					break;
				case CROSS:
					id = IDC_CROSS;
					break;
				case WAIT:
					id = IDC_WAIT;
					break;
			}

			::SetCursor(::LoadCursor(NULL, id));
		}
	}

	virtual void WarpCursor(const Int2& position) {
		::SetCursorPos(position.x(), position.y());
	}

	virtual void EnterMainLoop() {
		hdc = ::GetWindowDC(window);
		hglrc = wglCreateContext(hdc);

		isRendering = true;
		::SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)this);
		originalProc = (WNDPROC)::SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)HookProc);
		RECT rect;
		::GetWindowRect(window, &rect);
		windowSize = Int2(rect.right - rect.left, rect.bottom - rect.top);
		callback->OnInitialize(this);
		callback->OnWindowSize(windowSize);
		::WaitForSingleObject(mainLoopEvent, INFINITE);
		::SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)originalProc);
		isRendering = false;

		wglDeleteContext(hglrc);
		::ReleaseDC(window, hdc);
	}

	virtual void ExitMainLoop() {
		::SetEvent(mainLoopEvent);
	}

	virtual bool IsRendering() const {
		return isRendering;
	}

	inline Short2 MakePoint(LPARAM lParam) {
		POINT point;
		point.x = LOWORD(lParam);
		point.y = HIWORD(lParam);
		::ScreenToClient(window, &point);

		return Short2(safe_cast<short>(point.x), safe_cast<short>(point.y));
	}

	bool WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
			case WM_SIZE:
				{
					Short2 pt = MakePoint(lParam);
					callback->OnWindowSize(windowSize = Int2(pt.x(), pt.y()));
				}
				break;
			case WM_PAINT:
				wglMakeCurrent(hdc, hglrc);
				callback->OnRender();
				::InvalidateRect(hWnd, NULL, TRUE); // Prepare for next rendering
				break;
			case WM_KEYDOWN:
				callback->OnKeyboard(ConvertKeyMessage(safe_cast<UINT>(wParam), HIWORD(lParam), 0));
				break;
			case WM_KEYUP:
				callback->OnKeyboard(ConvertKeyMessage(safe_cast<UINT>(wParam), HIWORD(lParam), IFrame::EventKeyboard::KEY_POP));
				break;
			case WM_MOUSEWHEEL:
				callback->OnMouse(IFrame::EventMouse((short)HIWORD(wParam) > 0, false, false, true, MakePoint(lParam)));
				break;
			case WM_MOUSEMOVE:
				callback->OnMouse(IFrame::EventMouse(!!(wParam & MK_RBUTTON) || !!(wParam & MK_LBUTTON), true, !!(wParam & MK_RBUTTON), false, MakePoint(lParam)));
				break;
			case WM_LBUTTONDOWN:
				callback->OnMouse(IFrame::EventMouse(true, false, true, false, MakePoint(lParam)));
				break;
			case WM_LBUTTONUP:
				callback->OnMouse(IFrame::EventMouse(false, false, true, false, MakePoint(lParam)));
				break;
			case WM_RBUTTONDOWN:
				callback->OnMouse(IFrame::EventMouse(true, false, false, false, MakePoint(lParam)));
				break;
			case WM_RBUTTONUP:
				callback->OnMouse(IFrame::EventMouse(false, false, false, false, MakePoint(lParam)));
				break;
		}

		return true;
	}

	HWND window;
	HDC hdc;
	HGLRC hglrc;
	WNDPROC originalProc;
	Callback* callback;
	HANDLE mainLoopEvent;
	Int2 windowSize;
	bool isRendering;
	bool reserved[3];
};

LRESULT CALLBACK HookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ZFrameWin32AttachWindow* windowContext = reinterpret_cast<ZFrameWin32AttachWindow*>(::GetWindowLongPtrA(hWnd, GWLP_USERDATA));

	if (windowContext->WindowProc(hWnd, msg, wParam, lParam)) {
		return ::CallWindowProcW(windowContext->originalProc, hWnd, msg, wParam, lParam);
	} else {
		return 0;
	}
}

static void* InternalQueryBuiltinFactory(const char* key, const char* name) {
	if (strcmp(key, "IFrame") == 0 && strcmp(name, "ZFrameWin32AttachWindow") == 0) {
		static TFactoryConstruct<ZFrameWin32AttachWindow, IFrame> factory;
		return &factory;
	} else {
		return nullptr;
	}
}
#else
static void* InternalQueryBuiltinFactory(const char* key, const char* name) { return nullptr; }
#endif