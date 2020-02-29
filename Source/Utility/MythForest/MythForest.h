// MythForest.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-1
//

#ifndef __MYTHFOREST_H__
#define __MYTHFOREST_H__

#include "../SnowyStream/SnowyStream.h"
#include "../../Core/Interface/IScript.h"
#include "Entity.h"
#include "Engine.h"
#include "Component/Animation/AnimationComponentModule.h"
#include "Component/Batch/BatchComponentModule.h"
#include "Component/Camera/CameraComponentModule.h"
#include "Component/Compute/ComputeComponentModule.h"
#include "Component/EnvCube/EnvCubeComponentModule.h"
#include "Component/Event/EventListenerComponentModule.h"
#include "Component/Explorer/ExplorerComponentModule.h"
#include "Component/Field/FieldComponentModule.h"
#include "Component/Form/FormComponentModule.h"
#include "Component/Layout/LayoutComponentModule.h"
#include "Component/Light/LightComponentModule.h"
#include "Component/Model/ModelComponentModule.h"
#include "Component/Navigate/NavigateComponentModule.h"
#include "Component/Particle/ParticleComponentModule.h"
#include "Component/Phase/PhaseComponentModule.h"
#include "Component/Profile/ProfileComponentModule.h"
#include "Component/Remote/RemoteComponentModule.h"
#include "Component/RenderFlow/RenderFlowComponentModule.h"
#include "Component/Shape/ShapeComponentModule.h"
#include "Component/Sound/SoundComponentModule.h"
#include "Component/Space/SpaceComponentModule.h"
#include "Component/Surface/SurfaceComponentModule.h"
#include "Component/Terrain/TerrainComponentModule.h"
#include "Component/TextView/TextViewComponentModule.h"
#include "Component/Transform/TransformComponentModule.h"
#include "Component/Visibility/VisibilityComponentModule.h"
#include "Component/Widget/WidgetComponentModule.h"

namespace PaintsNow {

	namespace NsMythForest {
		class MythForest : public TReflected<MythForest, IScript::Library> {
		public:
			MythForest(Interfaces& interfaces, NsSnowyStream::SnowyStream& snowyStream, NsBridgeSunset::BridgeSunset& bridgeSunset);
			virtual ~MythForest();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			// static int main(int argc, char* argv[]);
			virtual void TickDevice(IDevice& device) override;
			virtual void Initialize() override;
			virtual void Uninitialize() override;
			void OnSize(const Int2& size);
			void OnMouse(const IFrame::EventMouse& mouse);
			void OnKeyboard(const IFrame::EventKeyboard& keyboard);
			TShared<Entity> CreateEntity(int32_t warp = 0);
			Engine& GetEngine();

		public:
			void RequestEnumerateComponentModules(IScript::Request& request);

			// Entity-Component System APIs.
			void RequestNewEntity(IScript::Request& request, int32_t warp);
			void RequestAddEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component);
			void RequestRemoveEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component);
			void RequestUpdateEntity(IScript::Request& request, IScript::Delegate<Entity> entity);
			void RequestGetEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity);
			void RequestGetEntityComponentDetails(IScript::Request& request, IScript::Delegate<Entity> entity);
			void RequestGetComponentType(IScript::Request& request, IScript::Delegate<Component> component);
			void RequestGetUniqueEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, const String& componentName);
			void RequestClearEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity);
			void RequestGetFrameTickTime(IScript::Request& request);
			void RequestRaycast(IScript::Request& request, IScript::Delegate<Entity> entity, const Float3& from, const Float3& dir, uint32_t count);

			// Build-in sub modules
		private:
			Engine engine;
			TShared<Entity::Allocator> entityAllocator;

			AnimationComponentModule animationComponentModule;
			BatchComponentModule batchComponentModule;
			CameraComponentModule cameraComponentModule;
			ComputeComponentModule computeComponentModule;
			EnvCubeComponentModule envCubeComponentModule;
			EventListenerComponentModule eventListenerComponentModule;
			ExplorerComponentModule explorerComponentModule;
			FieldComponentModule fieldComponentModule;
			FormComponentModule formComponentModule;
			LayoutComponentModule layoutComponentModule;
			LightComponentModule lightComponentModule;
			ModelComponentModule modelComponentModule;
			NavigateComponentModule navigateComponentModule;
			ParticleComponentModule particleComponentModule;
			PhaseComponentModule phaseComponentModule;
			ProfileComponentModule profileComponentModule;
			RemoteComponentModule remoteComponentModule;
			RenderFlowComponentModule renderFlowComponentModule;
			ShapeComponentModule shapeComponentModule;
			SoundComponentModule soundComponentModule;
			SpaceComponentModule spaceComponentModule;
			SurfaceComponentModule surfaceComponentModule;
			TerrainComponentModule terrainComponentModule;
			TextViewComponentModule textViewComponentModule;
			TransformComponentModule transformComponentModule;
			VisibilityComponentModule visibilityComponentModule;
			WidgetComponentModule widgetComponentModule;

			uint64_t lastFrameTick;
			uint64_t currentFrameTime;
		};
	}
}


#endif // __MYTHFOREST_H__
