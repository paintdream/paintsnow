// TextViewComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __TEXTVIEWCOMPONENT_H__
#define __TEXTVIEWCOMPONENT_H__

#include "../../Component/Renderable/RenderableComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/FontResource.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"
#include <sstream>

namespace PaintsNow {
	namespace NsMythForest {
		class TextViewComponent : public TAllocatedTiny<TextViewComponent, RenderableComponent> {
		public:
			enum {
				TEXTVIEWCOMPONENT_SELECT_REV_COLOR = COMPONENT_CUSTOM_BEGIN,
				TEXTVIEWCOMPONENT_CURSOR_REV_COLOR = COMPONENT_CUSTOM_BEGIN << 1,
			};

			TextViewComponent(TShared<NsSnowyStream::MaterialResource> materialResource);
			virtual ~TextViewComponent();
			virtual uint32_t CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) override;
			virtual void Initialize(Engine& engine, Entity* entity) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
		
			struct Descriptor {
				Descriptor(int16_t h, int16_t fs);
				int16_t totalWidth;
				int16_t yCoord;
				int16_t firstOffset;

				struct Char {
					Char(int16_t c = 0, int16_t off = 0);
					int16_t xCoord;
					int16_t offset;
				};

				std::vector<Char> allOffsets;
			};

			int32_t Locate(Short2& rowCol, const Short2& pt, bool isPtRowCol) const;
			void SetText(Engine& engine, const String& text);
			void Scroll(const Short2& pt);
			void SetUpdateMark();

		protected:
			void SetSize(Engine& engine, const Short2& size);
			void UpdateRenderData(Engine& engine);
			void RenderCharacter(std::stringstream& stream, const Short2Pair& rect, const Short2Pair& uv, const Float4& color, uint32_t fontSize);

			uint32_t GetLineCount() const;
			void SetPasswordChar(int ch);
			bool IsEmpty() const;
			const Short2& GetSize() const;
			const Short2& GetFullSize() const;
			void SetPadding(const Short2& padding);
			Short2 SelectText(const Short2Pair& offsetRect) const;
			Short2 Fix(int offset) const;

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

				std::vector<Node> nodes;
			};

			TagParser parser;
			std::vector<Descriptor> lines;

		protected:
			IRender::Resource* unitCoordBuffer;

		public:
			TShared<NsSnowyStream::FontResource> fontResource;
			TShared<NsSnowyStream::MaterialResource> materialResource;
			String text;

			Short2 size;
			Short2 scroll;
			Short2 fullSize;
			Short2 padding;
			int32_t passwordChar;
			int32_t cursorChar;
			int32_t cursorPos;
			uint32_t fontSize;
			Float4 cursorColor;
			Short2 selectRange;
			Float4 selectColor;
		};
	}
}


#endif // __TEXTVIEWCOMPONENT_H__