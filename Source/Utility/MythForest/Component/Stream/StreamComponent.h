// StreamComponent.h
// PaintDream (paintdream@paintdream.com)
// 2020-04-30
//

#ifndef __STREAMCOMPONENT_H__
#define __STREAMCOMPONENT_H__

#include "../../Component.h"

namespace PaintsNow {
	namespace NsMythForest {
		class StreamComponent : public TAllocatedTiny<StreamComponent, Component> {
		public:
			StreamComponent(const UShort3& dimension, uint32_t cacheCount);
			Component* Load(Engine& engine, const UShort3& coord);
			void Unload(Engine& engine, const UShort3& coord);
			void SetLoadHandler(IScript::Request& request, IScript::Request::Ref ref);
			void SetUnloadHandler(IScript::Request& request, IScript::Request::Ref ref);
			virtual void Uninitialize(Engine& engine, Entity* entity) override;

		protected:
			void ReplaceHandler(IScript::Request& request, IScript::Request::Ref& target, IScript::Request::Ref ref);

			struct Grid {
				TShared<Component> component;
				UShort3 coord;
			};

			UShort3 dimension;
			uint16_t reserved;
			std::vector<uint16_t> idGrids;
			std::vector<Grid> grids;

			IScript::Request::Ref loadHandler;
			IScript::Request::Ref unloadHandler;
		};
	}
}

#endif // __STREAMCOMPONENT_H__