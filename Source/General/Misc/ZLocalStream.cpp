#include "ZLocalStream.h"

using namespace PaintsNow;

ZLocalStream::ZLocalStream() : baseStream(nullptr), length(0) {}
ZLocalStream::~ZLocalStream() {
	Cleanup();
}

void ZLocalStream::Cleanup() {
	if (baseStream != nullptr) {
		baseStream->ReleaseObject();
	}
}

bool ZLocalStream::operator << (IStreamBase& stream) {
	Cleanup();

	stream >> length;
	baseStream = static_cast<IStreamBase*>(stream.Clone());
	assert(baseStream != nullptr);
	stream.Seek(IStreamBase::CUR, length); // skip the stream

	return true;
}

bool ZLocalStream::operator >> (IStreamBase& stream) const {
	assert(!payload.Empty());
	size_t len = payload.GetSize();
	stream << (uint32_t)len;
	return stream.Write(payload.GetData(), len);
}

bool ZLocalStream::Read(void* p, size_t& len) {
	assert(baseStream != nullptr);
	return baseStream->Read(p, len);
}

bool ZLocalStream::Transfer(IStreamBase& stream, size_t& len) {
	assert(baseStream != nullptr);
	return baseStream->Transfer(stream, len);
}

bool ZLocalStream::WriteDummy(size_t& len) {
	assert(baseStream != nullptr);
	return baseStream->WriteDummy(len);
}

bool ZLocalStream::Write(const void* p, size_t& len) {
	assert(baseStream != nullptr);
	return baseStream->Write(p, len);
}

void ZLocalStream::Flush() {
	assert(baseStream != nullptr);
	baseStream->Flush();
}

bool ZLocalStream::Seek(SEEK_OPTION option, long f) {
	assert(baseStream != nullptr);
	return baseStream->Seek(option, f);
}

Bytes& ZLocalStream::GetPayload() {
	return payload;
}

const Bytes& ZLocalStream::GetPayload() const {
	return payload;
}
