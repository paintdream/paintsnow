// FieldComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "FieldComponent.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class FieldComponent;
	class FieldComponentModule : public TReflected<FieldComponentModule, ModuleImpl<FieldComponent> > {
	public:
		FieldComponentModule(Engine& engine);
		~FieldComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create FieldComponent
		/// </summary>
		/// <returns> FieldComponent object </returns>
		TShared<FieldComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Initialize FieldComponent from a simplygon
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="shapeType"> shape type </param>
		void RequestFromSimplygon(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const String& shapeType);

		/// <summary>
		/// Initialize FieldComponent from a texture
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="textureResource"> the TextureResource </param>
		void RequestFromTexture(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<TextureResource> textureResource);

		/// <summary>
		/// Initialize FieldComponent from a mesh
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="meshResource"> the MeshResource </param>
		void RequestFromMesh(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<MeshResource> meshResource);

		/// <summary>
		/// Query field value
		/// </summary>
		/// <param name="fieldComponent"> the FieldComponent </param>
		/// <param name="position"> the query position </param>
		/// <returns></returns>
		String RequestQuery(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const Float3& position);
	};
}

