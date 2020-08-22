// CustomizeShader.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __CUSTOMIZESHADER_H
#define __CUSTOMIZESHADER_H

#include "../../../../Core/Interface/IType.h"
#include "../../../../Core/Interface/IReflect.h"

namespace PaintsNow {
	class ICustomizeShader : public IUniversalInterface {
	public:
		virtual void SetCode(const String& stage, const String& text, const std::vector<std::pair<String, String> >& config) = 0;
		virtual void SetInput(const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config) = 0;
		virtual void SetComplete() = 0;
	};
}

#endif // __CUSTOMIZESHADER_H