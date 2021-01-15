// PurpleTrail.h -- Asynchronous routine dispatching module for script
// By PaintDream (paintdream@paintdream.com)
// 2015-1-2
//


#pragma once
#include "../../Core/Interface/IScript.h"

namespace PaintsNow {
	class PurpleTrail : public TReflected<PurpleTrail, IScript::Library>, public ISyncObject {
	public:
		PurpleTrail(IThread& threadApi);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		~PurpleTrail() override;

	protected:
		/// <summary>
		/// Inspect a reflected object.
		/// </summary>
		/// <param name="d"> reflected object </param>
		/// <returns> reflect info </returns>
		void RequestInspect(IScript::Request& request, IScript::BaseDelegate d);
	};
}
