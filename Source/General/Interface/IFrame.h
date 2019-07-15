// IFrame.h -- Basic interface for app frame
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#ifndef __IFRAME_H__
#define __IFRAME_H__


#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IDevice.h"

namespace PaintsNow {
	class IFrame : public IDevice {
	public:
		struct EventMouse {
			EventMouse(bool d = true, bool m = false, bool l = true, bool w = false, const Short2& p = Short2(0, 0));
			bool down;
			bool move;
			bool left;
			bool wheel;
			Short2 position;
		};

		struct EventKeyboard {
			EventKeyboard(int key = 0);
			enum { KEY_ESCAPE = 27 };
			enum
			{
				KEY_SPECIAL = 0x10000, KEY_CTRL = 0x20000, KEY_ALT = 0x40000, KEY_SHIFT = 0x80000, KEY_POP = 0x100000,
				KEY_MASK = 0xffff,
				// KEY_RIGHT_CTRL = KEY_CTRL | KEY_SELECT_RIGHT, KEY_RIGHT_ALT = KEY_ALT | KEY_SELECT_RIGHT,
				// KEY_RIGHT_SHIFT = KEY_SHIFT | KEY_SELECT_RIGHT
			};

			enum
			{
				KEY_NONE = KEY_SPECIAL,
				KEY_F1 = KEY_SPECIAL | 0x1,
				KEY_F2 = KEY_SPECIAL | 0x2,
				KEY_F3 = KEY_SPECIAL | 0x3,
				KEY_F4 = KEY_SPECIAL | 0x4,
				KEY_F5 = KEY_SPECIAL | 0x5,
				KEY_F6 = KEY_SPECIAL | 0x6,
				KEY_F7 = KEY_SPECIAL | 0x7,
				KEY_F8 = KEY_SPECIAL | 0x8,
				KEY_F9 = KEY_SPECIAL | 0x9,
				KEY_F10 = KEY_SPECIAL | 0xA,
				KEY_F11 = KEY_SPECIAL | 0xB,
				KEY_F12 = KEY_SPECIAL | 0xC,
				KEY_LEFT = KEY_SPECIAL | 0xD,
				KEY_UP = KEY_SPECIAL | 0xE,
				KEY_RIGHT = KEY_SPECIAL | 0xF,
				KEY_DOWN = KEY_SPECIAL | 0x10,
				KEY_PAGE_UP = KEY_SPECIAL | 0x11,
				KEY_PAGE_DOWN = KEY_SPECIAL | 0x12,
				KEY_HOME = KEY_SPECIAL | 0x13,
				KEY_END = KEY_SPECIAL | 0x14,
				KEY_INSERT = KEY_SPECIAL | 0x15,
				KEY_DELETE = KEY_SPECIAL | 0x16,
				KEY_NUM_LOCK = KEY_SPECIAL | 0x17,
				KEY_BEGIN = KEY_SPECIAL | 0x18,
			};

			const char* GetName() const;
			unsigned long keyCode;
		};

		class Callback {
		public:
			virtual ~Callback();
			virtual void OnMouse(const EventMouse& mouse) = 0;
			virtual void OnKeyboard(const EventKeyboard& keyboard) = 0;
			virtual void OnRender() = 0;
			virtual void OnInitialize(void* param) = 0;
			virtual void OnWindowSize(const Int2& newSize) = 0;
			virtual bool IsRendering() const = 0;
		};

		virtual ~IFrame();
		virtual void SetCallback(Callback* callback) = 0;
		virtual const Int2& GetWindowSize() const = 0;
		virtual void SetWindowSize(const Int2& size) = 0;
		virtual void SetWindowTitle(const String& title) = 0;
		enum CURSOR { NONE, ARROW, CROSS, WAIT };
		virtual void ShowCursor(CURSOR cursor) = 0;
		virtual void WarpCursor(const Int2& position) = 0;
		virtual void EnterMainLoop() = 0;
		virtual void ExitMainLoop() = 0;
		virtual bool IsRendering() const = 0;
	};
}


#endif // __IFRAME_H__