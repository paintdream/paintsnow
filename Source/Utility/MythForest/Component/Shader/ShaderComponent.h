// ShaderComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"

namespace PaintsNow {
	class ShaderComponent : public TAllocatedTiny<ShaderComponent, Component> {
	public:
		ShaderComponent();

		void SetInput(const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config);
		void SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config);
		void SetComplete();
	};
}
