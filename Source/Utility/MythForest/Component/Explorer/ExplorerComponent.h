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
			class IExplorer {};

		protected:
			struct CollapsedComponent {
				TShared<Component> component;
				size_t mainMemoryCost;
				size_t deviceMemoryCost;
				size_t lastVisitTimeStamp;
			};

			std::vector<CollapsedComponent> collapsedComponents;
		};
	}
}


#endif // __EXPLORERCOMPONENT_H__