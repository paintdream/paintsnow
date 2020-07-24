#include "TapeComponent.h"
#include "../../../../Core/System/StringStream.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

TapeComponent::TapeComponent(IStreamBase& stream, TShared<SharedTiny> holder) : tape(stream), streamHolder(holder), bufferStream(4096) {}

std::pair<int64_t, String> TapeComponent::Read() {
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
	StringStream stream(const_cast<String&>(data));
	return tape.WritePacket(seq, stream, data.length());
}

bool TapeComponent::Seek(int64_t seq) {
	return tape.Seek(seq);
}
