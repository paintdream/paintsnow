// ZFrameFreeglut.h -- App frame based on FreeGLUT
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#ifndef __ZFRAMEFREEGLUT_H__
#define __ZFRAMEFREEGLUT_H__

#include "../../../../Core/PaintsNow.h"
#include "../../../Interface/IFrame.h"
#include <cstdlib>

namespace PaintsNow {
	class ZFrameFreeglut : public IFrame {
	public:
		ZFrameFreeglut(const Int2& size = Int2(800, 600), Callback* callback = nullptr);
		virtual void SetCallback(Callback* callback);
		virtual const Int2& GetWindowSize() const;
		virtual void SetWindowSize(const Int2& size);
		virtual void SetWindowTitle(const String& title);

		virtual bool IsRendering() const;
		virtual void OnMouse(const EventMouse& mouse);
		virtual void OnKeyboard(const EventKeyboard& keyboard);
		virtual void OnRender();
		virtual void OnWindowSize(const Int2& newSize);
		virtual void EnterMainLoop();
		virtual void ExitMainLoop();
		virtual void ShowCursor(CURSOR cursor);
		virtual void WarpCursor(const Int2& position);

	public:
		virtual void ChangeSize(int w, int h);
		virtual void SpecialKeys(int key, int x, int y);
		virtual void SpecialKeysUp(int key, int x, int y);
		virtual void KeyboardFunc(unsigned char cAscii, int x, int y);
		virtual void KeyboardFuncUp(unsigned char cAscii, int x, int y);
		virtual void MouseClickMessage(int button, int state, int x, int y);
		virtual void MouseMoveMessage(int x, int y);
		virtual void MouseWheelMessage(int wheel, int dir, int x, int y);
		virtual void MousePassiveMoveMessage(int x, int y);

	protected:
		Callback* callback;
		Int2 windowSize;
		bool isRendering;
		bool mainLoopStarted;
		bool reserved[2];
	};
}


#endif // __ZFRAMEFREEGLUT_H__