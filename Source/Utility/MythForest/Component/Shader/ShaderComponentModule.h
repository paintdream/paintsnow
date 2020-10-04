// ShaderComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "ShaderComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class ShaderComponent;
	class ShaderComponentModule : public TReflected<ShaderComponentModule, ModuleImpl<ShaderComponent> > {
	public:
		ShaderComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<ShaderComponent> RequestNew(IScript::Request& request, IScript::Delegate<ShaderResource> terrainResource);

		void RequestSetCode(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, const String& stage, const String& text, const std::vector<std::pair<String, String> >& config);
		void RequestSetInput(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderComponent, const String& stage, const String& type, const String& name, const String& value, const std::vector<std::pair<String, String> >& config);
		void RequestSetComplete(IScript::Request& request, IScript::Delegate<ShaderComponent> shaderResource);

	};
}
