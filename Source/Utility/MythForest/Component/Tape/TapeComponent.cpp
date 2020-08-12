#include "TapeComponent.h"
#include "../../../../Core/System/StringStream.h"
#include "../../../BridgeSunset/BridgeSunset.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

TapeComponent::TapeComponent(IStreamBase& stream, TShared<SharedTiny> holder, size_t cache) : tape(stream), streamHolder(holder), cacheBytes(cache), bufferStream(cache) {}

std::pair<int64_t, String> TapeComponent::Read() {
	assert(!(Flag() & TINY_UPDATING));
	if (Flag() & TINY_UPDATING) return std::make_pair(0, String());

	int64_t seq;
	int64_t length;
	bufferStream.Clear();

	if (tape.ReadPacket(seq, bufferStream, length)) {
		return std::make_pair(seq, String(reinterpret_cast<const char*>(bufferStream.GetBuffer()), (size_t)length));
	} else {
		return std::make_pair(0, String());
	}
}

bool TapeComponent::Write(int64_t seq, const String& data) {
	assert(!(Flag() & TINY_UPDATING));
	if (Flag() & TINY_UPDATING) return false;

	size_t len = data.length();
	if (!bufferStream.Write(data.c_str(), len)) {
		return false;
	}

	if (bufferStream.GetTotalLength() > cacheBytes) {
		// force flush
		return FlushInternal();
	} else {
		return true;
	}
}

bool TapeComponent::FlushInternal() {
	size_t i;
	for (i = 0; i < cachedSegments.size(); i++) {
		const std::pair<uint64_t, size_t>& p = cachedSegments[i];
		if (!tape.WritePacket(p.first, bufferStream, p.second)) {
			break;
		}
	}

	bool success = i == cachedSegments.size();
	bufferStream.Seek(IStreamBase::BEGIN, 0);
	cachedSegments.clear();
	return success;
}

bool TapeComponent::Seek(int64_t seq) {
	return tape.Seek(seq);
}

void TapeComponent::OnAsyncFlush(Engine& engine, IScript::Request::Ref callback) {
	bool result = FlushInternal();

	IScript::Request& request = *engine.bridgeSunset.AcquireSafe();
	request.DoLock();
	request.Call(sync, callback, result);
	request.UnLock();
	engine.bridgeSunset.ReleaseSafe(&request);

	Flag().fetch_and(~TINY_UPDATING);
	ReleaseObject();
}

bool TapeComponent::Flush(Engine& engine, IScript::Request::Ref callback) {
	if (callback) {
		Flag().fetch_or(TINY_UPDATING);
		ReferenceObject();

		engine.GetKernel().threadPool.Push(CreateTaskContextFree(Wrap(this, &TapeComponent::OnAsyncFlush), std::ref(engine), callback));
		return true;
	} else {
		return FlushInternal();
	}
}
