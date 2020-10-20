// Component.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "../../Core/Template/TAllocator.h"
#include "Unit.h"
#include "Event.h"

namespace PaintsNow {
	class ResourceManager;
	class Entity;
	class Component : public TReflected<Component, Unit> {
	public:
		typedef Component BaseComponent;
		enum {
			COMPONENT_LOCALIZED_WARP = UNIT_CUSTOM_BEGIN,
			COMPONENT_SHARED = UNIT_CUSTOM_BEGIN << 1,
			COMPONENT_ALIASED_TYPE = UNIT_CUSTOM_BEGIN << 2,
			COMPONENT_CUSTOM_BEGIN = UNIT_CUSTOM_BEGIN << 3,
		};

		struct RaycastResult {
			Float3 position; // local position
			Float3 normal;
			Float2 coord;
			float distance;
			TShared<Unit> unit;
			TShared<Unit> parent;
			Unique metaType;
			Bytes metaData;
		};

		class RaycastTask : public TReflected<RaycastTask, WarpTiny> {
		public:
			RaycastTask(Engine& engine, uint32_t maxCount);
			~RaycastTask() override;
			virtual void Finish(rvalue<std::vector<RaycastResult> > finalResult) = 0;
			bool EmplaceResult(rvalue<RaycastResult> item);
			bool EmplaceResult(std::vector<RaycastResult>& result, rvalue<RaycastResult> item);
			void AddPendingTask();
			void RemovePendingTask();
			Engine& GetEngine();

		protected:
			Engine& engine;
			uint32_t maxCount;
			std::atomic<uint32_t> pendingCount;
			std::vector<std::vector<RaycastResult> > results;
		};

		virtual void Initialize(Engine& engine, Entity* entity);
		virtual void Uninitialize(Engine& engine, Entity* entity);
		virtual void DispatchEvent(Event& event, Entity* entity);
		virtual Entity* GetHostEntity() const;
		virtual const String& GetAliasedTypeName() const;
		virtual void UpdateBoundingBox(Engine& engine, Float3Pair& boundingBox);
		virtual float Raycast(RaycastTask& task, Float3Pair& ray, Unit* parent, float ratio = 1) const;
		static void RaycastForEntity(RaycastTask& task, Float3Pair& ray, Entity* entity);
		virtual FLAG GetEntityFlagMask() const;
		virtual uint32_t GetQuickUniqueID() const;
		static inline uint32_t StaticGetQuickUniqueID() { return ~(uint32_t)0; }
	};

	enum UniqueQuickID {
		SLOT_TRANSFORM_COMPONENT = 0,
		SLOT_FORM_COMPONENT,
		SLOT_EXPLORER_COMPONENT,
		SLOT_ANIMATION_COMPONENT,
		SLOT_LAYOUT_COMPONENT,
		SLOT_VISIBILITY_COMPONENT
	};

	template <class T, uint32_t quickID>
	class UniqueComponent : public TReflected<UniqueComponent<T, quickID>, T> {
	public:
		UniqueComponent() { T::Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_relaxed); }
		static inline uint32_t StaticGetQuickUniqueID() { return quickID; }

	private:
		uint32_t GetQuickUniqueID() const final { return StaticGetQuickUniqueID(); }
	};
}

