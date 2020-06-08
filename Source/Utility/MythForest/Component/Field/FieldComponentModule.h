// FieldComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __FIELDCOMPONENTMODULE_H__
#define __FIELDCOMPONENTMODULE_H__

#include "FieldComponent.h"
#include "../../../SnowyStream/Resource/TextureResource.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class FieldComponent;
		class FieldComponentModule  : public TReflected<FieldComponentModule, ModuleImpl<FieldComponent> > {
		public:
			FieldComponentModule(Engine& engine);
			virtual ~FieldComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<FieldComponent> RequestNew(IScript::Request& request);
			void RequestFromSimplygon(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const String& shapeType);
			void RequestFromTexture(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<NsSnowyStream::TextureResource> textureResource);
			void RequestFromMesh(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<NsSnowyStream::MeshResource> meshResource);
			String RequestQuery(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const Float3& position);
		};
	}
}


#endif // __FIELDCOMPONENTMODULE_H__