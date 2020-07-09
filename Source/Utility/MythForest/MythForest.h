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
#include "Module.h"

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
			void StartCaptureFrame(const String& path, const String& options);
			void EndCaptureFrame();
			void InvokeCaptureFrame(const String& path, const String& options);

			TShared<Entity> CreateEntity(int32_t warp = 0);
			Engine& GetEngine();

		public:
			void RequestEnumerateComponentModules(IScript::Request& request);

			// Entity-Component System APIs.
			TShared<Entity> RequestNewEntity(IScript::Request& request, int32_t warp);
			void RequestAddEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component);
			void RequestRemoveEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component);
			void RequestUpdateEntity(IScript::Request& request, IScript::Delegate<Entity> entity);
			void RequestGetEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity);
			void RequestGetEntityComponentDetails(IScript::Request& request, IScript::Delegate<Entity> entity);
			String RequestGetComponentType(IScript::Request& request, IScript::Delegate<Component> component);
			TShared<Component> RequestGetUniqueEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, const String& componentName);
			void RequestClearEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity);
			uint64_t RequestGetFrameTickTime(IScript::Request& request);
			void RequestRaycast(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Request::Ref callback, const Float3& from, const Float3& dir, uint32_t count);
			void RequestCaptureFrame(IScript::Request& request, const String& path, const String& options);

			// Build-in sub modules
		private:
			Engine engine;
			TShared<Entity::Allocator> entityAllocator;
			uint64_t lastFrameTick;
			uint64_t currentFrameTime;
		};
	}
}


#endif // __MYTHFOREST_H__
