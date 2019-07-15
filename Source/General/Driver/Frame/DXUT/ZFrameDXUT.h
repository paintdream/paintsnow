// ZFrameDXUT.h -- App frame based on FreeGLUT
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#ifndef __ZFRAMEDXUT_H__
#define __ZFRAMEDXUT_H__

#ifdef CMAKE_PAINTSNOW

#include "../../../Core/PaintsNow.h"
#include "../../../Interface/IFrame.h"
#include "Core/DXUT.h"
#include "Core/DXUTcamera.h"
#include "Core/SDKmisc.h"
#include <cstdlib>

namespace PaintsNow {
	class ZFrameDXUT : public IFrame {
	public:
		ZFrameDXUT(const Int2& size = Int2(512, 512), Callback* callback = nullptr);
		virtual void SetCallback(Callback* callback);
		virtual const Int2& GetWindowSize() const;
		virtual void SetWindowSize(const Int2& size);
		virtual void SetWindowTitle(const String& title);

		virtual void OnMouse(const EventMouse& mouse);
		virtual void OnKeyboard(const EventKeyboard& keyboard);
		virtual void OnRender();
		virtual void OnWindowSize(const Int2& newSize);
		virtual void OnInitialize(void* param);
		virtual void EnterMainLoop();
		virtual void ExitMainLoop();
		virtual void FireRender();
		virtual void ShowCursor(CURSOR cursor);
		virtual void WarpCursor(const Int2& position);

	private:
		Callback* callback;
		String windowTitle;
		Int2 windowSize;
		HWND hWnd;
		bool mainLoopStarted;
	};
}

#endif

#endif // __ZFRAMEDXUT_H__