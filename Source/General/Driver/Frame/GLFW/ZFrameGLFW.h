// ZFrameGLFW.h -- App frame based on GLFW
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once
#include "../../../../Core/PaintsNow.h"
#include "../../../Interface/IFrame.h"
#include <cstdlib>

struct GLFWwindow;
namespace PaintsNow {
	class ZFrameGLFW : public IFrame {
	public:
		ZFrameGLFW(GLFWwindow** windowPtr, bool isVulkan = false, const Int2& size = Int2(800, 600), Callback* callback = nullptr);
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
		virtual void OnMouseButtonCallback(int button, int action, int mods);
		virtual void OnMouseMoveCallback(double x, double y);
		virtual void OnScrollCallback(double x, double y);
		virtual void OnFrameBufferSizeCallback(int width, int height);
		virtual void OnKeyboardCallback(int key, int scancode, int action, int mods);
		virtual void OnCustomRender();

	protected:
		GLFWwindow* window;
		Callback* callback;
		Int2 windowSize;
		bool isRendering;
		bool isVulkan;
		bool lastbutton;
		bool lastdown;
	};
}

