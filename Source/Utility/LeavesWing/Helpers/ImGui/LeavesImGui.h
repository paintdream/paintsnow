// LeavesImGui.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-3
//

#pragma once

#include "../../../../Core/PaintsNow.h"

#if (!defined(_MSC_VER) || _MSC_VER > 1200) && ADD_FRAME_GLFW
#define USE_LEAVES_IMGUI 1
#else
#define USE_LEAVES_IMGUI 0
#endif

#if USE_LEAVES_IMGUI

#include "IWidget.h"
#include "../../../../General/Driver/Frame/GLFW/ZFrameGLFW.h"
#include <vector>

namespace PaintsNow {
	class LeavesImGui {
	public:
		LeavesImGui(LeavesFlute*& leavesFlute);
		void AddWidget(IWidget* widget);
		void TickRender();

	protected:
		std::vector<IWidget*> widgets;
		LeavesFlute*& leavesFlute;
	};

	class ZFrameGLFWForImGui : public ZFrameGLFW {
	public:
		ZFrameGLFWForImGui(LeavesImGui& imgui, const Int2& size = Int2(1450, 800), Callback* callback = nullptr);
		virtual void OnMouseButtonCallback(int button, int action, int mods) override;		virtual void OnScrollCallback(double x, double y) override;
		virtual void OnKeyboardCallback(int key, int scancode, int action, int mods) override;
		virtual void OnCustomRender() override;

	protected:
		LeavesImGui& leavesImGui;
		bool init;
	};
}

#endif

