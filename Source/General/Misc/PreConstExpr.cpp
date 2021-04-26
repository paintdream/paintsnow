#include "PreConstExpr.h"
using namespace PaintsNow;

bool PreConstExpr::Evaluate(const char* begin, const char* end) const {
	// TODO: evaluate
	return true;
}

String PreConstExpr::operator () (const String& text) const {
	// find 'if constexpr'
	String target;
	size_t lastPos = 0;
	size_t length = text.length();
	while (true) {
		size_t pos = text.find("if constexpr");
		if (pos == String::npos) {
			target.append(text.begin() + lastPos, text.end());
			return target;
		}

		// search condition
		bool findCondition = false;
		bool evalCondition = false;
		bool findIfPart = false;
		bool findElsePart = false;

		uint32_t parenthesesCount = 0;
		uint32_t bracesCount = 0;
		uint32_t conditionStart = 0;
		uint32_t partStart = 0;

		for (size_t i = pos; i < length; i++) {
			char ch = text[i];
			switch (ch) {
			case '(':
				if (parenthesesCount++ == 0) {
					if (!findCondition) {
						conditionStart = i;
					}
				}
				break;
			case ')':
				if (--parenthesesCount == 0) {
					if (!findCondition) {
						// TODO: evaluate condition
						evalCondition = Evaluate(text.data() + conditionStart, text.data() + i);
						findCondition = true;
					}
				}
				break;
			case '{':
				if (bracesCount++ == 0) {
					partStart = i + 1;
				}

				break;
			case '}':
				if (--bracesCount == 0) {
					if (findCondition) {
						if (findIfPart) {
							// do 
							findIfPart = true;
						} else {
						}
					}
				}
			}
		}
	}
}
