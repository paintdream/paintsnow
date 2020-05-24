#include "TextViewComponent.h"
#include "../../MythForest.h"

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

	PushText(safe_cast<uint32_t>(q - start), q, p);
}

void TextViewComponent::TagParser::PushReturn(uint32_t offset) {
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

void TextViewComponent::TagParser::PushFormat(uint32_t offset, const char* start, const char* end) {
	const char* valueString;
	bool isClose;

	if (ParseAttrib(valueString, isClose, start, end, "color")) {
		if (isClose) {
			nodes.emplace_back(Node(Node::COLOR_CLOSED, offset));
		} else {
			if (valueString != nullptr) {
				nodes.emplace_back(Node(Node::COLOR, (uint32_t)(valueString - start) + offset));
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

		nodes.emplace_back(Node(t, (uint32_t)(valueString - start) + offset));
	}
}

void TextViewComponent::TagParser::PushText(uint32_t offset, const char* start, const char* end) {
	if (start != end) {
		nodes.emplace_back(Node(Node::TEXT, offset, safe_cast<uint32_t>(end - start)));
	}
}

void TextViewComponent::TagParser::Clear() {
	nodes.clear();
}

TextViewComponent::TextViewComponent(TShared<MaterialResource> material) : materialResource(material), unitCoordBuffer(nullptr), passwordChar(0), cursorChar(0), cursorPos(0), fontSize(12) {
	Flag() |= (TEXTVIEWCOMPONENT_CURSOR_REV_COLOR | TEXTVIEWCOMPONENT_SELECT_REV_COLOR);
}

void TextViewComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = engine.mythForest.GetWarpResourceQueue();

	unitCoordBuffer = render.CreateResource(queue, IRender::Resource::RESOURCE_BUFFER);
}

void TextViewComponent::Uninitialize(Engine& engine, Entity* entity) {
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = engine.mythForest.GetWarpResourceQueue();
	render.DeleteResource(queue, unitCoordBuffer);
	unitCoordBuffer = nullptr;

	BaseClass::Uninitialize(engine, entity);
}

TextViewComponent::~TextViewComponent() {}

const Short2& TextViewComponent::GetFullSize() const {
	return fullSize;
}

void TextViewComponent::Scroll(const Short2& pt) {
	scroll = pt;
}

void TextViewComponent::SetText(Engine& engine, const String& t) {
	text = t;
	parser.Parse(text.data(), text.data() + text.size());

	UpdateRenderData(engine);
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

TextViewComponent::Descriptor::Descriptor(int16_t h, int16_t s) : totalWidth(0), firstOffset(h) {}
TextViewComponent::Descriptor::Char::Char(int16_t c, int16_t off) : xCoord(c), offset(off) {}

void TextViewComponent::UpdateRenderData(Engine& engine) {
	if (!fontResource) {
		return;
	}

	// Update buffers
	std::vector<Float4> bufferData;
	IFontBase& fontBase = engine.interfaces.fontBase;
	IRender& render = engine.interfaces.render;

	Short2 fullSize;
	int ws = size.x();
	int h = fontSize + padding.y();
	assert(h != 0);

	Short2 start = scroll;
	int count = 0;
	int currentWidth = -start.x();
	int maxWidth = 0;
	int currentHeight = -start.y();

	// if (currentWidth < 0) currentWidth = 0;
	// if (currentHeight < 0) currentHeight = 0;
	bool full = false;
	bool showCursor = false;
	bool selectRevColor = !!(Flag() & TEXTVIEWCOMPONENT_SELECT_REV_COLOR);
	bool cursorRevColor = !!(Flag() & TEXTVIEWCOMPONENT_CURSOR_REV_COLOR);
	int align = TagParser::Node::ALIGN_LEFT;
	Short2 texSize;
	fontResource->GetFontTexture(fontSize, texSize);

	const FontResource::Char& cursor = fontResource->Get(fontBase, cursorChar, fontSize);
	Short2 current;
	const FontResource::Char& pwd = fontResource->Get(fontBase, passwordChar, fontSize);
	Float4 color(1, 1, 1, 1);
	lines.emplace_back(Descriptor(currentHeight, 0));
	FontResource::Char info;
	std::stringstream ss;

	for (size_t i = 0; i < parser.nodes.size(); i++) {
		const TagParser::Node& node = parser.nodes[i];
		if (node.type == TagParser::Node::TEXT) {
			int step = 1;
			for (const char* p = text.data() + node.offset; p < text.data() + node.offset + node.length; p += step) {
				// use utf-8 as default encoding
				step = GetUtf8Size(*p);
				info = passwordChar != 0 ? pwd : fontResource->Get(fontBase, Utf8ToUnicode((const unsigned char*)p, step), fontSize);

				int temp = info.info.adv.x() + padding.x();
				currentWidth += temp;
				maxWidth = maxWidth > currentWidth ? maxWidth : currentWidth;

				if (currentHeight > size.y() - h) {
					full = true;
					//	break;
				}

				if (temp >= ws) {
					break;
				}

				if (currentWidth + start.x() >= ws) {
					lines.emplace_back(Descriptor(currentHeight, node.offset));
					currentWidth = -start.x();
					currentHeight += h;
					p -= step;

					count++;
					continue;
				}

				int16_t offset = (int16_t)(p - text.data());
				lines.back().allOffsets.emplace_back(Descriptor::Char(currentWidth - temp / 2, offset));
				lines.back().totalWidth = currentWidth + start.x();

				if (!full && currentHeight >= 0) {
					int wt = info.info.width;
					int ht = info.info.height;
					int centerOffset = (temp - info.info.width) / 2;
					int alignOffset = (size_t)count >= this->lines.size() ? 0 : align == TagParser::Node::ALIGN_LEFT ? 0 : align == TagParser::Node::ALIGN_CENTER ? (ws - this->lines[count].totalWidth) / 2 : (ws - this->lines[count].totalWidth);
					current = Short2(currentWidth - temp + alignOffset + info.info.bearing.x(), currentHeight + (h - ht) - info.info.delta.y());

					Float4 c = color;
					if (selectRange.x() <= offset && selectRange.y() > offset) {
						if (selectRevColor) {
							c = Float4(1.0, 1.0, 1.0, 1.0) - color;
							c.a() = color.a();
						} else {
							c = selectColor;
						}
					}

					Short2 end(current.x() + wt, current.y() + ht);
					RenderCharacter(ss, Short2Pair(current, end), info.rect, c, fontSize);

					// if cursor ?
					if (!showCursor && cursorPos <= offset && cursorChar != 0) {
						current.y() += info.info.delta.y() - cursor.info.delta.y() + ht - cursor.info.height;
						// current.x() -= cursor.info.width;
						if (cursorRevColor) {
							c = Float4(1.0, 1.0, 1.0, 1.0) - color;
						} else {
							c = cursorColor;
						}

						Short2 m(current.x() + cursor.info.width, current.y() + cursor.info.height);
						RenderCharacter(ss, Short2Pair(current, m), cursor.rect, c, fontSize);
						showCursor = true;
					}
				}
			} // end for

			if (full) break;
		} else if (node.type == TagParser::Node::ALIGN_LEFT || node.type == TagParser::Node::ALIGN_RIGHT || node.type == TagParser::Node::ALIGN_CENTER) {
			align = node.type;
		} else if (node.type == TagParser::Node::RETURN) {
			lines.emplace_back(Descriptor(start.x(), (int16_t)node.offset));
			currentWidth = -start.x();
			currentHeight += h;
			count++;
		} else if (node.type == TagParser::Node::COLOR) {
			// TODO: render!
			int value = 0;
			sscanf(text.data() + node.offset, "%x", &value);
			color = Float4((float)((value >> 16) & 0xff) / 255, (float)((value >> 8) & 0xff) / 255, (float)(value & 0xff) / 255, 1);
		}
	}

	// if cursor ?
	if (!full && currentHeight >= 0 && !showCursor && cursorChar != 0) {
		Float4 c;
		current.x() += info.info.width;
		current.y() += info.info.delta.y() - cursor.info.delta.y() + info.info.height - cursor.info.height;
		// current.x() -= cursor.info.width;
		if (cursorRevColor) {
			c = Float4(1.0, 1.0, 1.0, 1.0) - color;
		} else {
			c = cursorColor;
		}

		Short2 m(current.x() + cursor.info.width, current.y() + cursor.info.height);
		RenderCharacter(ss, Short2Pair(current, m), cursor.rect, c, fontSize);
		showCursor = true;
	}

	count++;
	currentHeight += h;

	fullSize.x() = maxWidth + start.x();
	fullSize.y() = currentHeight + start.y();

	// do update
}

uint32_t TextViewComponent::CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) {

	// TODO: migrate the following code here.
	return 0;
}

void TextViewComponent::RenderCharacter(std::stringstream& stream, const Short2Pair& rect, const Short2Pair& uv, const Float4& color, uint32_t fontSize) {
	Short2 fontTexSize;
	IRender::Resource* texture = fontResource->GetFontTexture(fontSize, fontTexSize);
	if (texture != nullptr) {
		Float4 pos((float)rect.first.x(), (float)(size.y() - rect.second.y()), (float)rect.second.y(), (float)(size.y() - rect.first.y()));
		Float4 tex = Float4(
			(float)uv.first.x() / fontTexSize.x(),
			(float)uv.first.y() / fontTexSize.y(),
			(float)uv.second.x() / fontTexSize.x(),
			(float)uv.second.y() / fontTexSize.y()
		);

		// Add triangles
		Float4 item[4] = {
			Float4(pos[0], pos[1], tex[0], tex[1]),
			Float4(pos[2], pos[1], tex[2], tex[1]),
			Float4(pos[2], pos[3], tex[2], tex[3]),
			Float4(pos[0], pos[3], tex[0], tex[3]),
		};

		stream.write((const char*)item, sizeof(item));
	}

	// TODO: texture change
}

uint32_t TextViewComponent::GetLineCount() const {
	return safe_cast<uint32_t>(lines.size());
}

void TextViewComponent::SetPasswordChar(int ch) {
	passwordChar = ch;
}

void TextViewComponent::SetSize(Engine& engine, const Short2& s) {
	size = s;
	UpdateRenderData(engine);
}

const Short2& TextViewComponent::GetSize() const {
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
	bool operator () (const TextViewComponent::Descriptor& desc, const Short2& pt) {
		return desc.yCoord < pt.y();
	}
};

struct LocatePos {
	bool operator () (const TextViewComponent::Descriptor::Char& desc, const Short2& pt) {
		return desc.xCoord < pt.x();
	}
};

int32_t TextViewComponent::Locate(Short2& rowCol, const Short2& pt, bool isPtRowCol) const {
	if (lines.empty()) {
		rowCol = Short2(0, 0);
		return 0;
	}

	if (isPtRowCol) {
		const Descriptor& desc = lines[rowCol.y()];
		rowCol.x() = Max((int16_t)0, Min((int16_t)(lines.size() - 1), pt.x()));
		rowCol.y() = Max((int16_t)0, Min((int16_t)(desc.allOffsets.size()), pt.y()));

		if (rowCol.y() == desc.allOffsets.size()) {
			return safe_cast<uint32_t>(text.size());
		} else {
			return desc.allOffsets[rowCol.y()].offset;
		}
	} else {
		std::vector<Descriptor>::const_iterator p = std::lower_bound(lines.begin(), lines.end(), pt, LocateLine());
		if (p == lines.end()) {
			--p;
		}
		rowCol.x() = (int16_t)(p - lines.begin());

		std::vector<TextViewComponent::Descriptor::Char>::const_iterator t = std::lower_bound(p->allOffsets.begin(), p->allOffsets.end(), pt, LocatePos());

		rowCol.y() = (int16_t)(t - p->allOffsets.begin());
		if (t == p->allOffsets.end()) {
			std::vector<Descriptor>::const_iterator q = p;
			q++;
			if (q != lines.end() && !(*q).allOffsets.empty()) {
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