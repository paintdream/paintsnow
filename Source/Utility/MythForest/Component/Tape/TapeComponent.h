// TapeComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#ifndef __TAPECOMPONENT_H__
#define __TAPECOMPONENT_H__

#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/System/Tape.h"
#include "../../../../Core/System/MemoryStream.h"

namespace PaintsNow {
	namespace NsMythForest {
		class TapeComponent : public TAllocatedTiny<TapeComponent, Component> {
		public:
			TapeComponent(IStreamBase& streamBase, TShared<SharedTiny> streamHolder);

			std::pair<int64_t, String> Read();
			bool Write(int64_t seq, const String& data);
			bool Seek(int64_t seq);

		protected:
			Tape tape;
			MemoryStream bufferStream;
			TShared<SharedTiny> streamHolder;
		};
	}
}


#endif // __TAPECOMPONENT_H__