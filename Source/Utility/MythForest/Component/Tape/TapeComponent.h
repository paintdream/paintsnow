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
			TapeComponent(IStreamBase& streamBase, TShared<SharedTiny> streamHolder, size_t cacheBytes);

			std::pair<int64_t, String> Read();
			bool Write(int64_t seq, const String& data);
			bool Seek(int64_t seq);
			bool Flush(Engine& engine, IScript::Request::Ref callback);

		protected:
			bool FlushInternal();
			void OnAsyncFlush(Engine& engine, IScript::Request::Ref callback);

			size_t cacheBytes;
			Tape tape;
			MemoryStream bufferStream;
			TShared<SharedTiny> streamHolder;
			std::vector<std::pair<uint64_t, size_t> > cachedSegments;
		};
	}
}


#endif // __TAPECOMPONENT_H__