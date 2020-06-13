// Module.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-15
//

#ifndef __MODULE_H__
#define __MODULE_H__

#include "../../General/Interface/Interfaces.h"
#include "Entity.h"
#include "Component.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Engine;
		class Module : public TReflected<Module, IScript::Library> {
		public:
			Module(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual Unique GetTinyUnique() const;
			virtual Component* GetEntityUniqueComponent(Entity* entity) const;
			virtual void TickFrame();

		protected:
			Engine& engine;
		};

		template <class T>
		class ModuleImpl : public TReflected<ModuleImpl<T>, Module> {
		public:
			typedef TReflected<ModuleImpl<T>, Module> BaseClass;
			ModuleImpl(Engine& engine) : BaseClass(engine) {
				allocator = TShared<typename T::Allocator>::From(new typename T::Allocator());
			}
			virtual Unique GetTinyUnique() const {
				return UniqueType<T>::Get();
			}

			virtual Component* GetEntityUniqueComponent(Entity* entity) const {
				return entity->GetUniqueComponent(UniqueType<T>());
			}

			TShared<typename T::Allocator> allocator;
		};
	}
}

#endif // __MODULE_H__