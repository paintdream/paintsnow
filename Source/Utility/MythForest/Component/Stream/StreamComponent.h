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
			StreamComponent(const UShort3& dimension, uint16_t cacheCount);
			SharedTiny* Load(Engine& engine, const UShort3& coord, TShared<SharedTiny> context);
			void Unload(Engine& engine, const UShort3& coord, TShared<SharedTiny> context);
			void SetLoadHandler(IScript::Request& request, IScript::Request::Ref ref);
			void SetLoadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> >& handler);
			void SetUnloadHandler(IScript::Request& request, IScript::Request::Ref ref);
			void SetUnloadHandler(const TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> >& handler);
			virtual void Uninitialize(Engine& engine, Entity* entity) override;
			const UShort3& GetDimension() const;

		protected:
			struct Grid {
				TShared<SharedTiny> object;
				UShort3 coord;
				uint16_t recycleIndex;
			};

			void UnloadInternal(Engine& engine, Grid& grid, TShared<SharedTiny> context);

			UShort3 dimension;
			uint16_t recycleStart;
			std::vector<uint16_t> idGrids;
			std::vector<Grid> grids;
			std::vector<uint16_t> recycleQueue;

			template <class T>
			struct Handler {
				void ReplaceScript(IScript::Request& request, IScript::Request::Ref ref) {
					if (script != ref && script) {
						request.DoLock();
						request.Dereference(script);
						script = ref;
						request.UnLock();
					}
				}

				T native;
				IScript::Request::Ref script;
			};

			Handler<TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> > > loadHandler;
			Handler<TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> > > unloadHandler;
		};
	}
}

#endif // __STREAMCOMPONENT_H__