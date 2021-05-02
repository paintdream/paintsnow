// ZFrameAndroid.h
// PaintDream (paintdream@paintdream.com)
// 2021-05-01
//

#pragma once

#include "../../../../../../../Source/General/Interface/IFrame.h"
struct android_app;

namespace PaintsNow {
	class ZFrameAndroid : public IFrame {
	public:
		void SetCallback(Callback* callback) override;
		const Int2& GetWindowSize() const override;
		void SetWindowSize(const Int2& size) override;
		void SetWindowTitle(const String& title) override;
		void EnableVerticalSynchronization(bool enable) override;
		void ShowCursor(CURSOR cursor) override;
		void WarpCursor(const Int2& position) override;
		void EnterMainLoop() override;
		void ExitMainLoop() override;
		bool IsRendering() const override;

	private:
		Int2 windowSize;
		android_app* app;
	};
}