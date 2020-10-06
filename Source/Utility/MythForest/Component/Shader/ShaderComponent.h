// ShaderComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/ShaderResource.h"
#include "../../../SnowyStream/Resource/Passes/CustomMaterialPass.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	class ShaderComponent : public TAllocatedTiny<ShaderComponent, Component> {
	public:
		ShaderComponent();

		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;

		void SetInput(Engine& engine, const String& stage, const String& type, const String& name, const String& value, const std::vector<std::pair<String, String> >& config);
		void SetCode(Engine& engine, const String& stage, const String& code, const std::vector<std::pair<String, String> >& config);
		void SetComplete(Engine& engine);
		void SetCallback(IScript::Request& request, IScript::Request::Ref callback);

	protected:
		void OnShaderCompiled(IRender::Resource::ShaderDescription&, IRender::Resource::ShaderDescription::Stage, const String&, const String&);

	protected:
		TShared<ShaderResourceImpl<CustomMaterialPass> > customMaterialShader;
		IScript::Request::Ref compileCallbackRef;
		IRender::Resource* compilingShaderResource;
	};
}
