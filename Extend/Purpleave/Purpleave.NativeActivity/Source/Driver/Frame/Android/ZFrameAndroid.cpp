#include "ZFrameAndroid.h"
using namespace PaintsNow;

void ZFrameAndroid::SetCallback(Callback* callback) {}
const Int2& ZFrameAndroid::GetWindowSize() const { return windowSize; }
void ZFrameAndroid::SetWindowSize(const Int2& size) { windowSize = size; }
void ZFrameAndroid::SetWindowTitle(const String& title) {}
void ZFrameAndroid::EnableVerticalSynchronization(bool enable) {}
void ZFrameAndroid::ShowCursor(CURSOR cursor) {}
void ZFrameAndroid::WarpCursor(const Int2& position) {}
void ZFrameAndroid::EnterMainLoop() {}
void ZFrameAndroid::ExitMainLoop() {}

bool ZFrameAndroid::IsRendering() const {
	return true;
}
