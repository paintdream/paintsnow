// ExplorerComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __EXPLORERCOMPONENT_H__
#define __EXPLORERCOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Space/SpaceComponent.h"

namespace PaintsNow {
	namespace NsMythForest {
		// Level of details controller
		class ExplorerComponent : public TAllocatedTiny<ExplorerComponent, Component> {
		public:
			ExplorerComponent();
			virtual ~ExplorerComponent();

			virtual void Initialize(Engine& engine, Entity* entity);
			virtual void Uninitialize(Engine& engine, Entity* entity);

			// Forever Internet Explorer!
			class IExplorer {
			public:
				virtual void Explore(size_t index, Component* component, bool activated) = 0;
				virtual Bytes Finalize() = 0;
			};

			void CollectActiveComponents(Engine& engine, Entity* entity, IExplorer& explorer);

		protected:
			struct CollapsedComponent {
				TShared<Component> component;
				// TODO: other attributes...
			};

			std::vector<CollapsedComponent> collapsedComponents;

#ifdef _DEBUG
			Entity* hostEntity;
#endif
		};
	}
}


#endif // __EXPLORERCOMPONENT_H__