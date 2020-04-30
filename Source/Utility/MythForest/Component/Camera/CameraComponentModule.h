// CameraComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __CAMERACOMPONENTMODULE_H__
#define __CAMERACOMPONENTMODULE_H__

#include "CameraComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class RenderFlowComponent;
		class EventListenerComponentModule;
		class CameraComponentModule  : public TReflected<CameraComponentModule, ModuleImpl<CameraComponent> > {
		public:
			CameraComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			// Interfaces
			void RequestNew(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& cameraViewPortName);
			void RequestGetCollectedEntityCount(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
			void RequestGetCollectedVisibleEntityCount(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
			void RequestBindRootEntity(IScript::Request& request, IScript::Delegate<CameraComponent> camera, IScript::Delegate<Entity> entity);
			void RequestComputeCastRayFromPoint(IScript::Request& request, IScript::Delegate<CameraComponent> camera, Float2& screenPosition);
			void RequestSetPerspective(IScript::Request& request, IScript::Delegate<CameraComponent> camera, float d, float n, float f, float r);
			void RequestGetPerspective(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
			void RequestSetVisibleDistance(IScript::Request& request, IScript::Delegate<CameraComponent> camera, float distance);
			void RequestGetVisibleDistance(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
			void RequestSetProjectionJitter(IScript::Request& request, IScript::Delegate<CameraComponent> camera, bool jitter);
			void RequestSetSmoothTrack(IScript::Request& request, IScript::Delegate<CameraComponent> camera, bool smoothTrack);
		};
	}
}


#endif // __CAMERACOMPONENTMODULE_H__
