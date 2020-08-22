// CameraComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "CameraComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class RenderFlowComponent;
	class EventComponentModule;
	class CameraComponentModule : public TReflected<CameraComponentModule, ModuleImpl<CameraComponent> > {
	public:
		CameraComponentModule(Engine& engine);
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Interfaces
		TShared<CameraComponent> RequestNew(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& cameraViewPortName);
		uint32_t RequestGetCollectedEntityCount(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
		uint32_t RequestGetCollectedVisibleEntityCount(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
		uint32_t RequestGetCollectedTriangleCount(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
		void RequestBindRootEntity(IScript::Request& request, IScript::Delegate<CameraComponent> camera, IScript::Delegate<Entity> entity);
		void RequestComputeCastRayFromPoint(IScript::Request& request, IScript::Delegate<CameraComponent> camera, Float2& screenPosition);
		void RequestSetPerspective(IScript::Request& request, IScript::Delegate<CameraComponent> camera, float d, float n, float f, float r);
		void RequestGetPerspective(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
		void RequestSetVisibleDistance(IScript::Request& request, IScript::Delegate<CameraComponent> camera, float distance);
		float RequestGetVisibleDistance(IScript::Request& request, IScript::Delegate<CameraComponent> camera);
		void RequestSetProjectionJitter(IScript::Request& request, IScript::Delegate<CameraComponent> camera, bool jitter);
		void RequestSetSmoothTrack(IScript::Request& request, IScript::Delegate<CameraComponent> camera, bool smoothTrack);
	};
}
