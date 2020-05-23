#include "TextViewComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

static inline int GetUtf8Size(char c) {
	int t = 1 << 7;
	int r = c;
	int count = 0;
	while (r & t) {
		r = r << 1;
		count++;
	}

	return count == 0 ? 1 : count;
}

void TextViewComponent::TagParser::Parse(const char* start, const char* end) {
	Clear();

	bool less = false;
	const char* q = nullptr;
	const char* p = nullptr;
	bool slash = false;
	for (p = start, q = start; p != end; ++p) {
		int i = GetUtf8Size(*p);
		if (i != 1) {
			p += i - 1;
			continue;
		}

		if (*p == '\\') {
			slash = true;
		} else {
			uint32_t offset = safe_cast<uint32_t>(q - start);
			if (*p == '<' && !slash) {
				PushText(offset, q, p);
				less = true;
				q = p + 1;
			} else if (*p == '>' && less && !slash) {
				assert(q != nullptr);
				PushFormat(offset, q, p);
				less = false;
				q = p + 1;
			} else if (*p == '\n' && !less) {
				PushText(offset, q, p);
				PushReturn(offset);
				q = p + 1;
			}

			slash = false;
		}
	}

	PushText(q - start, q, p);
}

void TextViewComponent::TagParser::PushReturn(size_t offset) {
	nodes.emplace_back(Node(Node::RETURN, offset));
}

bool TextViewComponent::TagParser::ParseAttrib(const char*& valueString, bool& isClose, const char* start, const char* end, const char* attrib) {
	assert(attrib != nullptr);
	bool ret = false;
	valueString = nullptr;
	isClose = false;
	const uint32_t len = safe_cast<uint32_t>(strlen(attrib));
	const char* format = start;

	if ((size_t)(end - start) >= len && memcmp(format, attrib, len) == 0) {
		// find "="
		const char* t;
		for (t = format + len; *t != '\0'; ++t) {
			if (*t == '=')
				break;
		}

		while (*t != '\0' && !isalnum(*t)) t++;

		if (*t != '\0')
			valueString = start + (t - format);

		ret = true;
	} else if (format[0] == '/' && (size_t)(end - start) >= len + 1 && memcmp(format + 1, attrib, len) == 0) {
		isClose = true;
		ret = true;
	}

	return ret;
}

void TextViewComponent::TagParser::PushFormat(size_t offset, const char* start, const char* end) {
	const char* valueString;
	bool isClose;

	if (ParseAttrib(valueString, isClose, start, end, "color")) {
		if (isClose) {
			nodes.emplace_back(Node(Node::COLOR_CLOSED, offset));
		} else {
			if (valueString != nullptr) {
				nodes.emplace_back(Node(Node::COLOR, valueString - start + offset));
			}
		}
	} else if (ParseAttrib(valueString, isClose, start, end, "align")) {
		Node::TYPE t = Node::ALIGN_LEFT;
		static const char* right = "right";
		static const char* center = "center";
		if ((size_t)(end - valueString) >= strlen(right) && memcmp(valueString, right, strlen(right)) == 0) {
			t = Node::ALIGN_RIGHT;
		} else if ((size_t)(end - valueString) >= strlen(center) && memcmp(valueString, center, strlen(center)) == 0) {
			t = Node::ALIGN_CENTER;
		}

		nodes.emplace_back(Node(t, valueString - start + offset));
	}
}

void TextViewComponent::TagParser::PushText(size_t offset, const char* start, const char* end) {
	if (start != end) {
		nodes.emplace_back(Node(Node::TEXT, offset, end - start));
	}
}

void TextViewComponent::TagParser::Clear() {
	nodes.clear();
}

TextViewComponent::TextViewComponent() : passwordChar(0), cursorChar(0), cursorPos(0), fontSize(12) {
	Flag() |= (TEXTVIEWCOMPONENT_CURSOR_REV_COLOR | TEXTVIEWCOMPONENT_SELECT_REV_COLOR);
}

TextViewComponent::~TextViewComponent() {}

const Int2& TextViewComponent::GetFullSize() const {
	return fullSize;
}

void TextViewComponent::Scroll(const Int2& pt) {
	scroll = pt;
}

void TextViewComponent::TextChange() {
	widthInfo = std::vector<Descriptor>();
}

void TextViewComponent::SetText(const String& t) {
	text = t;
	parser.Parse(text.data(), text.data() + text.size());

	TextChange();
}

static int Utf8ToUnicode(const unsigned char* s, int size) {
	char uc[] = { 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03 };
	unsigned long data = 0;
	unsigned long tmp = 0;
	int count = 0;

	const unsigned char* p = s;
	while (count < size) {
		unsigned char c = (*p);
		tmp = c;

		if (count == 0) {
			tmp &= uc[size - 1];
		} else {
			tmp &= 0x3f;
		}

		tmp = tmp << (6 * (size - count - 1));
		data |= tmp;

		p++;
		count++;
	}

	return data;
}

TextViewComponent::Descriptor::Descriptor(int h, int s) : totalWidth(0), firstOffset(h) {}
TextViewComponent::Descriptor::Char::Char(int c, int off) : xCoord(c), offset(off) {}

/*
void TextViewComponent::FireRender(IRender& render, WidgetPass& Pass, const Int2Pair& rect, const Int2& totalSize, const Int2& padding) {
	std::vector<Descriptor> info;
	PerformRender(render, rect, info, totalSize, padding, &Pass);
	std::swap(info, widthInfo);
}

Int2 TextViewComponent::PerformRender(IRender& render, const Int2Pair& range, std::vector<Descriptor>& widthRecords, const Int2& totalSize, const Int2& padding, WidgetPass* Pass) const {
	Int2 size;
	if (!mainFont) {
		return Int2(0, 0);
	}

	Int2 fullSize;
	// const double EPSILON = 1e-6;
	assert(parser != nullptr);

	int ws = range.second.x() - range.first.x(); //, wh = range.second.y() - range.first.y();
	int h = fontSize + padding.y();
	assert(h != 0);

	Int2 start = scroll;
	int count = 0;
	int currentWidth = -start.x();
	int maxWidth = 0;
	int currentHeight = -start.y();

	// if (currentWidth < 0) currentWidth = 0;
	// if (currentHeight < 0) currentHeight = 0;
	bool full = false;
	const TagParser* s = parser;

	int align = TagParser::NodeAlign::LEFT;

	Int2 texSize;
	mainFont->GetFontTexture(fontSize, texSize);

	const FontResource::Char& cursor = mainFont->Get(cursorChar, fontSize);
	Int2 current;
	const FontResource::Char& pwd = mainFont->Get(passwordChar, fontSize);
	Float4 color(1, 1, 1, 1);
	widthRecords.emplace_back(Descriptor(currentHeight, 0));
	FontResource::Char info;
	bool showCursor = false;

	for (std::list<TagParser::Node*>::const_iterator it = s->nodes.begin(); it != s->nodes.end(); ++it) {
		const TagParser::Node* node = *it;
		if (node->type == TagParser::TEXT) {
			const TagParser::NodeText* text = static_cast<const TagParser::NodeText*>(node);
			int size = 1;
			for (const char* p = text->text; p < text->text + text->length; p += size) {
				// use utf-8 as default encoding
				size = GetUtf8Size(*p);
				info = passwordChar != 0 ? pwd : mainFont->Get(Utf8ToUnicode((const unsigned char*)p, size), fontSize);

				int temp = info.info.adv.x() + padding.x();
				currentWidth += temp;
				maxWidth = maxWidth > currentWidth ? maxWidth : currentWidth;

				if (currentHeight > range.second.y() - range.first.y() - h) {
					full = true;
					//	break;
				}

				if (temp >= ws) {
					break;
				}

				if (currentWidth + start.x() >= ws) {
					widthRecords.emplace_back(Descriptor(currentHeight, (int)text->offset));
					currentWidth = -start.x();
					currentHeight += h;
					p -= size;

					count++;
					continue;
				}

				int offset = (int)(text->offset + (p - text->text));
				widthRecords.back().allOffsets.emplace_back(Descriptor::Char(currentWidth - temp / 2, offset));
				widthRecords.back().totalWidth = currentWidth + start.x();

				if (Pass != nullptr && !full && currentHeight >= 0) {
					int wt = info.info.width;
					int ht = info.info.height;
					int centerOffset = (temp - info.info.width) / 2;
					int alignOffset = (size_t)count >= this->widthInfo.size() ? 0 : align == TagParser::NodeAlign::LEFT ? 0 : align == TagParser::NodeAlign::CENTER ? (ws - this->widthInfo[count].totalWidth) / 2 : (ws - this->widthInfo[count].totalWidth);
					current = Int2(currentWidth - temp + range.first.x() + alignOffset + info.info.bearing.x(), currentHeight + range.first.y() + (h - ht) - info.info.delta.y());

					Float4 c = color;
					if (selectRange.x() <= offset && selectRange.y() > offset) {
						if (selectRevColor) {
							c = Float4(1.0, 1.0, 1.0, 1.0) - color;
							c.a() = color.a();
						} else {
							c = selectColor;
						}
					}

					Int2 end(current.x() + wt, current.y() + ht);
					RenderCharacter(render, *Pass, range, Int2Pair(current, end), info.rect, c, totalSize, fontSize);

					// if cursor ?
					if (!showCursor && cursorPos <= offset && cursorChar != 0) {
						current.y() += info.info.delta.y() - cursor.info.delta.y() + ht - cursor.info.height;
						// current.x() -= cursor.info.width;
						if (cursorRevColor) {
							c = Float4(1.0, 1.0, 1.0, 1.0) - color;
						} else {
							c = cursorColor;
						}

						Int2 m(current.x() + cursor.info.width, current.y() + cursor.info.height);
						RenderCharacter(render, *Pass, range, Int2Pair(current, m), cursor.rect, c, totalSize, fontSize);
						showCursor = true;
					}
				}
			} // end for

			if (full) break;
		} else if (node->type == TagParser::ALIGN) {
			TagParser::NodeAlign* p = (TagParser::NodeAlign*)node;
			align = p->align;
		} else if (node->type == TagParser::RETURN) {
			widthRecords.emplace_back(Descriptor(start.x(), (int)node->offset));
			currentWidth = -start.x();
			currentHeight += h;
			count++;
		} else if (node->type == TagParser::COLOR) {
			// TODO: render!
			if (Pass != nullptr) {
				const TagParser::NodeColor* c = static_cast<const TagParser::NodeColor*>(node);
				if (!c->IsClose()) {
					color = Float4((float)((c->value >> 16) & 0xff) / 255, (float)((c->value >> 8) & 0xff) / 255, (float)(c->value & 0xff) / 255, 1);
				}
			}
		}
	}

	// if cursor ?
	if (Pass != nullptr && !full && currentHeight >= 0 && !showCursor && cursorChar != 0) {
		Float4 c;
		current.x() += info.info.width;
		current.y() += info.info.delta.y() - cursor.info.delta.y() + info.info.height - cursor.info.height;
		// current.x() -= cursor.info.width;
		if (cursorRevColor) {
			c = Float4(1.0, 1.0, 1.0, 1.0) - color;
		} else {
			c = cursorColor;
		}

		Int2 m(current.x() + cursor.info.width, current.y() + cursor.info.height);
		RenderCharacter(render, *Pass, range, Int2Pair(current, m), cursor.rect, c, totalSize, fontSize);
		showCursor = true;
	}

	count++;
	currentHeight += h;

	fullSize.x() = maxWidth + start.x();
	fullSize.y() = currentHeight + start.y();

	return fullSize;
}

void TextViewComponent::RenderCharacter(IRender& render, WidgetPass& Pass, const Int2Pair& range, const Int2Pair& rect, const Int2Pair& info, const Float4& color, const Int2& totalSize, uint32_t fontSize) const {
	Int2 fontTexSize;
	IRender::Resource* texture = mainFont->GetFontTexture(fontSize, fontTexSize);
	if (texture != nullptr) {
		Int2Pair inv = rect;
		inv.first.y() = range.second.y() - inv.first.y() + range.first.y();
		inv.second.y() = range.second.y() - inv.second.y() + range.first.y();
		std::swap(inv.first.y(), inv.second.y());
		Float4 position = Float4(
			(float)rect.first.x() / totalSize.x(),
			1.0f - (float)rect.second.y() / totalSize.y(),
			(float)rect.second.x() / totalSize.x(),
			1.0f - (float)rect.first.y() / totalSize.y()
		);

		Float4 texCoord = Float4(
			(float)info.first.x() / fontTexSize.x(),
			(float)info.first.y() / fontTexSize.y(),
			(float)info.second.x() / fontTexSize.x(),
			(float)info.second.y() / fontTexSize.y()
		);

		Float4 texCoordMark(0.5f, 0.5f, 0.5f, 0.5f);
		Float4 texCoordScale(1, 1, 1, 1);

		Pass.widgetTransform.inputPositionRect = position * 2.0f - 1.0f;
		Pass.widgetTransform.inputTexCoordRect = texCoord;
		Pass.widgetTransform.inputTexCoordMark = texCoordMark;
		Pass.widgetTransform.inputTexCoordScale = texCoordScale;
		Pass.widgetShading.mainTexture.resource = texture;
		// Pass.FireRender(render);
	}
}
*/

int TextViewComponent::GetLineCount() const {
	return (int)widthInfo.size();
}

void TextViewComponent::SetPasswordChar(int ch) {
	passwordChar = ch;
}

void TextViewComponent::SetSize(const Int2& s) {
	size = s;
	TextChange();
}

const Int2& TextViewComponent::GetSize() const {
	return size;
}

struct LocateLineOffset {
	bool operator () (const TextViewComponent::Descriptor& desc, int offset) {
		return desc.firstOffset < offset;
	}
};

struct LocatePosOffset {
	bool operator () (const TextViewComponent::Descriptor::Char& desc, int offset) {
		return desc.offset < offset;
	}
};


struct LocateLine {
	bool operator () (const TextViewComponent::Descriptor& desc, const Int2& pt) {
		return desc.yCoord < pt.y();
	}
};

struct LocatePos {
	bool operator () (const TextViewComponent::Descriptor::Char& desc, const Int2& pt) {
		return desc.xCoord < pt.x();
	}
};

int32_t TextViewComponent::Locate(Int2& rowCol, const Int2& pt, bool isPtRowCol) const {
	if (widthInfo.empty()) {
		rowCol = Int2(0, 0);
		return 0;
	}

	if (isPtRowCol) {
		rowCol.x() = Max(0, Min((int)widthInfo.size() - 1, pt.x()));
		const Descriptor& desc = widthInfo[rowCol.y()];

		rowCol.y() = Max(0, Min((int)desc.allOffsets.size(), pt.y()));
		if (rowCol.y() == (int)desc.allOffsets.size()) {
			return safe_cast<uint32_t>(text.size());
		} else {
			return desc.allOffsets[rowCol.y()].offset;
		}
	} else {
		std::vector<Descriptor>::const_iterator p = std::lower_bound(widthInfo.begin(), widthInfo.end(), pt, LocateLine());
		if (p == widthInfo.end()) {
			--p;
		}
		rowCol.x() = (int)(p - widthInfo.begin());

		std::vector<TextViewComponent::Descriptor::Char>::const_iterator t = std::lower_bound(p->allOffsets.begin(), p->allOffsets.end(), pt, LocatePos());

		rowCol.y() = (int)(t - p->allOffsets.begin());
		if (t == p->allOffsets.end()) {
			std::vector<Descriptor>::const_iterator q = p;
			q++;
			if (q != widthInfo.end() && !(*q).allOffsets.empty()) {
				return q->firstOffset;
			} else {
				return safe_cast<uint32_t>(text.size());
			}
		} else {
			return t->offset;
		}
	}
}

bool TextViewComponent::IsEmpty() const {
	return text.empty();
}

void TextViewComponent::SetUpdateMark() {
	Flag() |= Tiny::TINY_MODIFIED;
}