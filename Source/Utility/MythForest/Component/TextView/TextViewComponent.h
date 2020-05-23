// TextViewComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __TEXTVIEWCOMPONENT_H__
#define __TEXTVIEWCOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/FontResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		class TextViewComponent : public TAllocatedTiny<TextViewComponent, Component> {
		public:
			enum {
				TEXTVIEWCOMPONENT_SELECT_REV_COLOR = COMPONENT_CUSTOM_BEGIN,
				TEXTVIEWCOMPONENT_CURSOR_REV_COLOR = COMPONENT_CUSTOM_BEGIN << 1,
			};
			TextViewComponent();
			virtual ~TextViewComponent();
		
			struct Descriptor {
				Descriptor(int h, int fs);
				int totalWidth;
				int yCoord;
				int firstOffset;

				struct Char {
					Char(int c = 0, int off = 0);
					int xCoord;
					int offset;
				};

				std::vector<Char> allOffsets;
			};

			int32_t Locate(Int2& rowCol, const Int2& pt, bool isPtRowCol) const;
			void SetText(const String& text);
			void Scroll(const Int2& pt);
			void SetUpdateMark();

		protected:
			int GetLineCount() const;
			void SetPasswordChar(int ch);
			void SetSize(const Int2& size);
			bool IsEmpty() const;
			const Int2& GetSize() const;
			const Int2& GetFullSize() const;
			void SetPadding(const Int2& padding);
			Int2 SelectText(const Int2Pair& offsetRect) const;
			void TextChange();
			Int2 Fix(int offset) const;

			class TagParser {
			public:
				struct Node {
					enum TYPE { TEXT = 0, RETURN, COLOR, COLOR_CLOSED, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER };

					Node(TYPE t, uint32_t off, uint32_t len = 0) : type(t), offset(off), length(0) {}
					virtual ~Node() {}

					TYPE type;
					uint32_t offset;
					uint32_t length;
				};

				~TagParser() {
					Clear();
				}

				void Parse(const char* start, const char* end);
				void PushReturn(uint32_t offset);
				bool ParseAttrib(const char*& valueString, bool& isClose, const char* start, const char* end, const char* attrib);
				void PushFormat(uint32_t offset, const char* start, const char* end);
				void PushText(uint32_t offset, const char* start, const char* end);
				void Clear();

			private:
				std::vector<Node> nodes;
			};

			TagParser parser;
			std::vector<Descriptor> widthInfo;

		public:
			TShared<NsSnowyStream::FontResource> mainFont;
			String text;
			Int2 size;
			Int2 scroll;
			Int2 fullSize;
			int passwordChar;
			int cursorChar;
			int cursorPos;
			uint32_t fontSize;
			Float4 cursorColor;
			Int2 selectRange;
			Float4 selectColor;
		};
	}
}


#endif // __TEXTVIEWCOMPONENT_H__