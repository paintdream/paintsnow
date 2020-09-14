// Zipper.h
// By PaintDream (paintdream@paintdream.com)
// 2018-8-4
//

#pragma once
#include "../../Core/System/Kernel.h"
#include "../../Core/Interface/IArchive.h"

namespace PaintsNow {
	class Zipper : public TReflected<Zipper, WarpTiny> {
	public:
		Zipper(IArchive* archive, IStreamBase* streamBase);
		~Zipper() override;

	protected:
		IArchive* archive;
		IStreamBase* streamBase;
	};
}

