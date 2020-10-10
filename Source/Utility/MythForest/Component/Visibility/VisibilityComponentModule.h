// VisibilityComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "VisibilityComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class VisibilityComponent;
	class VisibilityComponentModule : public TReflected<VisibilityComponentModule, ModuleImpl<VisibilityComponent> > {
	public:
		VisibilityComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create VisibilityComponent
		/// </summary>
		/// <returns> VisibilityComponent object </returns>
		TShared<VisibilityComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Setup parameters of VisibilityComponent
		/// </summary>
		/// <param name="visibilityComponent"> the VisibilityComponent </param>
		/// <param name="maxDistance"> max view distance </param>
		/// <param name="range"> effect range </param>
		/// <param name="division"> division count </param>
		/// <param name="frameTimeLimit"> not implemented by now </param>
		/// <param name="taskCount"> render task limit of single render frame</param>
		/// <param name="resolution"> visibility texture resolution </param>
		void RequestSetup(IScript::Request& request, IScript::Delegate<VisibilityComponent> visibilityComponent, float maxDistance, const Float3Pair& range, const UShort3& division, uint32_t frameTimeLimit, uint32_t taskCount, const UShort2& resolution);
	};
}
