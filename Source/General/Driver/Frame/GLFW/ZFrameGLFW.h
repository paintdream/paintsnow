// ZFrameGLFW.h -- App frame based on GLFW
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#ifndef __ZFRAMEGLFW_H__
#define __ZFRAMEGLFW_H__

#include "../../../../Core/PaintsNow.h"
#include "../../../Interface/IFrame.h"
#include <cstdlib>

struct GLFWwindow;
namespace PaintsNow {
	class ZFrameGLFW : public IFrame {
	public:
		ZFrameGLFW(const Int2& size = Int2(800, 600), Callback* callback = nullptr);
		virtual ~ZFrameGLFW();
		virtual void SetCallback(Callback* callback);
		virtual const Int2& GetWindowSize() const;
		virtual void SetWindowSize(const Int2& size);
		virtual void SetWindowTitle(const String& title);

		virtual bool IsRendering() const;
		virtual void EnterMainLoop();
		virtual void ExitMainLoop();

		void OnMouse(const EventMouse& mouse);
		void OnKeyboard(const EventKeyboard& keyboard);
		void OnWindowSize(const EventSize& newSize);
		void ShowCursor(CURSOR cursor);
		void WarpCursor(const Int2& position);

	public:
		virtual void mouse_button_callback(int button, int action, int mods);
		virtual void cursor_position_callback(double x, double y);
		virtual void scroll_callback(double x, double y);
		virtual void framebuffer_size_callback(int width, int height);
		virtual void key_callback(int key, int scancode, int action, int mods);
		virtual void render();
		virtual void init();

	protected:
		GLFWwindow* window;
		Callback* callback;
		Int2 windowSize;
		bool isRendering;
		bool mainLoopStarted;
		bool lastbutton;
		bool lastdown;
	};
}


#endif // __ZFRAMEGLFW_H__