// Entity.h
// By PaintDream (paintdream@paintdream.com)
// 2017-12-27
//

#pragma once
#include "../../Core/Template/TKdTree.h"
#include "../../Core/Interface/IType.h"
#include "Unit.h"
#include "Event.h"
#include "Component.h"
#include <set>

namespace PaintsNow {
	class Engine;

	// The entity object is exactly 64 bytes on 32-bit platforms (MSVC Release).
	class_aligned(64) Entity final : public TAllocatedTiny<Entity, TKdTree<Float3Pair, Unit> >{
	public:
		Entity(Engine & engine);
		~Entity() override;

		enum {
			ENTITY_HAS_BEGIN = UNIT_CUSTOM_BEGIN,
			ENTITY_HAS_TICK_EVENT = ENTITY_HAS_BEGIN,
			ENTITY_HAS_PREPOST_TICK_EVENT = ENTITY_HAS_BEGIN << 1,
			ENTITY_HAS_TACH_EVENT = ENTITY_HAS_BEGIN << 2,
			ENTITY_HAS_ACTIVE_EVENT = ENTITY_HAS_BEGIN << 3,
			ENTITY_HAS_SPECIAL_EVENT = ENTITY_HAS_BEGIN << 4,
			ENTITY_HAS_RENDERABLE = ENTITY_HAS_BEGIN << 5,
			ENTITY_HAS_RENDERCONTROL = ENTITY_HAS_BEGIN << 6,
			ENTITY_HAS_SPACE = ENTITY_HAS_BEGIN << 7,
			ENTITY_HAS_END = ENTITY_HAS_BEGIN << 8,
			ENTITY_STORE_ENGINE = ENTITY_HAS_END,
			ENTITY_STORE_NULLSLOT = ENTITY_HAS_END << 1,
			ENTITY_HAS_ALL = ENTITY_HAS_END - ENTITY_HAS_BEGIN,
		};

		void AddComponent(Engine& engine, Component* component);
		void RemoveComponent(Engine& engine, Component* component);
		void ClearComponents(Engine& engine);
		void UpdateBoundingBox(Engine& engine);
		void Activate(Engine& engine);
		void Deactivate(Engine& engine);

		void UpdateEntityFlags();
		Component* GetUniqueComponent(Unique unique) const;
		void PostEvent(Event& event, FLAG mask);
		bool IsOrphan() const;
		void SetEngineInternal(Engine& engine);
		void CleanupEngineInternal();
		uint32_t GetPivotIndex() const;

		const std::vector<Component*>& GetComponents() const;
		template <class T>
		T* GetUniqueComponent(UniqueType<T> type) const {
			const uint32_t id = T::StaticGetQuickUniqueID();
			if (id != ~(uint32_t)0) {
				if (id < components.size()) {
					Component* component = components[id];
					if (component == nullptr) return nullptr;

					T* ret = component->QueryInterface(UniqueType<T>());
					if (ret != nullptr) {
						assert(ret->Flag() & Tiny::TINY_UNIQUE);
					}
					return ret;
				} else {
					return nullptr;
				}
			} else {
				return static_cast<T*>(GetUniqueComponent(UniqueType<T>::Get()));
			}
		}

		void ReleaseObject() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		inline Entity* Left() const {
			// assert(!(Flag() & ENTITY_STORE_ENGINE));
			return static_cast<Entity*>(leftNode);
		}

		inline Entity*& Left() {
			return reinterpret_cast<Entity*&>(leftNode);
		}

		inline Entity* Right() const {
			// assert(!(Flag() & ENTITY_STORE_ENGINE));
			return static_cast<Entity*>(rightNode);
		}

		inline Entity*& Right() {
			return reinterpret_cast<Entity*&>(rightNode);
		}

		inline Entity* Parent() const {
			assert(!(Flag() & ENTITY_STORE_ENGINE));
			return static_cast<Entity*>(GetParent());
		}

	protected:
		Engine& GetEngineInternal() const;
		void InitializeComponent(Engine& engine, Component* component);
		void UninitializeComponent(Engine& engine, Component* component);

	protected:
		std::vector<Component*> components;
	};
}

