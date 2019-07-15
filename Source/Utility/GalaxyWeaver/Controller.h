// Controller.h
// By PaintDream
// 2016-3-22
//

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include "ProxyStub.h"
#include "../../General/Interface/IAsset.h"

namespace PaintsNow {
	namespace NsGalaxyWeaver {
		class Weaver;
		class Controller : public TReflected<Controller, ProxyStub>, public IScript::Remote<Weaver> {
		public:
			Controller(IThread& threadApi, ITunnel& tunnel, const String& entry);
			virtual ~Controller();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&> CheckVersion;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, const String&, const String&, const String&> PostResource;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&> Complete;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, uint32_t, const String&> PostEntity;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, const String&> PostEntityGroup;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, uint32_t> PostEntityComponent;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, const String&, float> PostModelComponent;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, uint32_t, const String&> PostModelComponentMaterial;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, const Float3&, const Float3&, const Float3&> PostTransformComponent;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, uint32_t> PostSpaceComponent;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, const String&> PostEnvCubeComponent;
			TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, const MatrixFloat4x4&, const Float4&> UpdateView;
		};
	}
}

#endif // __CONTROLLER_H__