// PreConstExpr.h
// PaintDream (paintdream@paintdream.com
// 2021-04-26
//

#pragma once

#include "../../Core/PaintsNow.h"
#include "../Interface/IAsset.h"

namespace PaintsNow {
	class PreConstExpr {
	public:
		String operator () (const String& text) const;

		std::vector<IAsset::Material::Variable> variables;

	private:
		bool Evaluate(const char* begin, const char* end) const;
	};
}