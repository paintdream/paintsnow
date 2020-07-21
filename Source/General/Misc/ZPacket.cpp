#include "ZPacket.h"
#include "../../Core/Template/TAlgorithm.h"
#include <cassert>

using namespace PaintsNow;

ZPacket::ZPacket(IStreamBase& s) : stream(s), location(0), maxLocation(0), lastOffset(0)/*, totalPadding(0)*/ {
}

ZPacket::~ZPacket() {
}

#ifndef _DEBUG
#define verify(f) (f);
#else
#define verify(f) \
	assert((f));
#endif

inline int ZPacket::FullLength(int64_t location, int64_t& length) {
	for (int c = 0, d; c < 32; c++) {
		if (c >= (d = (int)Math::SiteCount(location >> PACKAGE_MIN_ALIGN_LEVEL, (int64_t)((location + length + c * sizeof(PacketHeader) + PACKAGE_MIN_ALIGN - 1) >> PACKAGE_MIN_ALIGN_LEVEL)))) {
			length += c * sizeof(PacketHeader);
			// printf("Site Count %d\n", d);
			return c - d;
		}
	}

	assert(false);
	return 0;
}

bool ZPacket::WritePacket(int64_t seq, IStreamBase& source, uint32_t length) {
	// static uint8_t filldummy[PACKAGE_MIN_ALIGN - sizeof(PacketHeader)];
	// find aligned point
	int64_t fullLength = (int64_t)length + sizeof(length);
	int padding = FullLength(location, fullLength);
//	totalPadding += (sizeof(PacketHeader) << padding);
	PacketHeader header;
	header.firstMark = 1;
	header.padding = padding;
	header.seq = seq;
//	int64_t savedLocation = location;

	int64_t alignedEnd = location + ((fullLength + PACKAGE_MIN_ALIGN - 1) & ~(PACKAGE_MIN_ALIGN - 1));
	int64_t mid = Math::AlignmentRange(location, alignedEnd);
	int64_t remaining = alignedEnd;

 	while (length > 0) {
		uint32_t alignment = safe_cast<uint32_t>(location < mid ? Math::Alignment(location | mid) : Math::AlignmentTop(remaining - mid));
		if (location >= mid) remaining -= alignment;

		//printf("Header at %p offset = %d, alignment = %p\n", (uint32_t)location, lastOffset, alignment);
		assert(alignment != 0);
		// write atmost alignment - sizeof(header?) bytes
		uint32_t extra = header.firstMark ? sizeof(length) : 0;
		uint32_t full = alignment - sizeof(PacketHeader) - extra;
		uint32_t size = Math::Min(length, full);

		header.offset = lastOffset;

		// write header
		size_t wl = sizeof(PacketHeader);
		if (!stream.Write((const uint8_t*)&header, wl)) {
			return false;
		}

		// write length for first packet
		uint32_t len = safe_cast<uint32_t>(fullLength);
		wl = sizeof(len);
		if (extra != 0 && !stream.Write((const uint8_t*)&len, wl)) {
			return false;
		}

		// prepare data
		wl = size;
		if (!source.Transfer(stream, wl)) {
			return false;
		}

		header.firstMark = 0;
		length -= size;
		location += alignment;
		lastOffset = Math::Log2(alignment);

		if (length == 0) {
			wl = (size_t)(alignedEnd - location + full - size);
			if (!stream.WriteDummy(wl)) {
				return false;
			}

			location = alignedEnd;
		}
	}

	/*
	long diff = (long)Math::Alignment(location);
	if (!stream.Seek(IStreamBase::CUR, -diff)) {
		return false;
	}

	if (!stream.Read(&header, sizeof(header))) {
		return false;
	}

	assert(header.seq != 0);
	if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header) + diff)) {
		return false;
	}*/

//	assert(location == stream.GetOffset());
	maxLocation = location;
	return true;
}

bool ZPacket::ReadPacket(int64_t& seq, IStreamBase& target, uint32_t& totalLength) {
	if (location >= maxLocation)
		return false;

	PacketHeader header;
	uint32_t length = 0;
	totalLength = 0;
	// bool type = false;
	int64_t alignedEnd = 0;
	int64_t mid = 0;
	int64_t remaining = 0;

	do {
		size_t rl = sizeof(PacketHeader);
		if (!stream.Read(&header, rl)) {
			return false;
		}

		// assert(header.firstMark);

		if (length == 0) {
			assert(header.firstMark);
			rl = sizeof(length);
			if (!stream.Read(&length, rl)) {
				return false;
			} else {
				seq = header.seq;
				// totalLength = length;
				alignedEnd = location + ((length + PACKAGE_MIN_ALIGN - 1) & ~(PACKAGE_MIN_ALIGN - 1));
				length -= safe_cast<uint32_t>(header.padding * sizeof(PacketHeader));
				mid = Math::AlignmentRange(location, alignedEnd);
				remaining = alignedEnd;
			}
		}

		uint32_t alignment = safe_cast<uint32_t>(location < mid ? Math::Alignment(location | mid) : Math::AlignmentTop(remaining - mid));
		if (location >= mid) remaining -= alignment;
		uint32_t extra = header.firstMark ? sizeof(length) : 0;
		assert(length > sizeof(PacketHeader) + extra);
		uint32_t size = Math::Min(length, alignment) - (sizeof(PacketHeader) + extra);

		assert((int64_t)size > 0);
		// read content
		rl = size;
		if (!stream.Transfer(target, rl)) {
			return false;
		}

		if (!stream.Seek(IStreamBase::CUR, alignment - size - sizeof(PacketHeader) - extra)) {
			return false;
		}

		totalLength += size;
		length -= size + sizeof(PacketHeader) + extra;
		location += alignment;
//		assert(location == stream.GetOffset());
		lastOffset = header.offset;
	} while (length > 0);

	return true;
}

bool ZPacket::Seek(int64_t seq) {
	if (maxLocation == 0)
		return true;

	int64_t left = 0, right = maxLocation;
	PacketHeader header;

	if (location == maxLocation) {
		// select step type
		long diff = (long)Math::Alignment(location);
		location -= diff;
		if (!stream.Seek(IStreamBase::CUR, -diff)) {
			return false;
		}
	}

	bool rewind = false;
	bool rewindDone = false;
	while (left < right) {
		size_t rl = sizeof(header);
		if (!stream.Read(&header, rl)) {
			return false;
		}

		// evaluate
		long diff = (long)Math::Alignment(location);

		if (header.seq < seq && !rewind) {
			// to right
			if (!rewindDone && diff != 0 && diff + location < right) {
				left = location;
				location += diff;
				if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header) + diff)) {
					return false;
				}
			} else if (header.firstMark) {
				uint32_t length;
				rl = sizeof(length);
				if (!stream.Read(&length, rl)) {
					return false;
				}

				uint32_t alignedEnd = ((length + PACKAGE_MIN_ALIGN - 1) & ~(PACKAGE_MIN_ALIGN - 1));
				location += alignedEnd;
				if (!stream.Seek(IStreamBase::CUR, (long)alignedEnd - sizeof(length) - sizeof(header))) {
					return false;
				}

				left = location;
			} else {
				if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header))) {
					return false;
				}
				rewind = true;
				rewindDone = false;
			}
		} else {
			if (!rewind) {
				right = location;
				if (right == left) {
					if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header))) {
						return false;
					}

					break;
				}
			}

			// to left
			if (rewind && header.firstMark) {
				if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header))) {
					return false;
				}
				rewind = false;
				rewindDone = true;
			} else if (!rewind && location - diff >= left) {
				assert(diff != 0);
				location -= diff;
				if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header) - diff)) {
					return false;
				}
			} else {
				// step back
				long move = (1 << header.offset);
				assert((move & (PACKAGE_MIN_ALIGN - 1)) == 0);
				location -= move;

				if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header) - move)) {
					return false;
				}

			}
		}

		// assert(location == stream.GetOffset());
	}

	// assert(location == stream.GetOffset());
	return true;
}


inline int64_t ZPacket::GetMaxLocation() const {
	return maxLocation;
}

/*
bool ZPacket::Seek(int64_t seq) {
	seq--;
	if (maxLocation == 0)
		return true;

	// plain search

	assert((location & (PACKAGE_MIN_ALIGN - 1)) == 0);
	PacketHeader header;


	// drop one util meets
	while (true) {
		// select step type
		long diff = (long)Math::Alignment(location);
		location -= diff;
		if (!stream.Seek(IStreamBase::CUR, -diff)) {
			return false;
		}

		if (!stream.Read(&header, sizeof(header))) {
			return false;
		}

		assert(header.seq != 0);

		if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header))) {
			return false;
		}

		assert(location == stream.GetOffset());

		if (location == 0 || header.seq < seq) {
			break;
		}
	}

	assert((location & (PACKAGE_MIN_ALIGN - 1)) == 0);
	// search for front package header
	if (location != 0) {
		while (true) {
			if (!stream.Read(&header, sizeof(header))) {
				return false;
			}

			assert(header.seq != 0);
			if (header.firstMark) { // find
				if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header))) {
					return false;
				}
				break;
			} else {
				long move = (1 << header.offset);
				assert((move & (PACKAGE_MIN_ALIGN - 1)) == 0);
				location -= move;

				if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header) - move)) {
					return false;
				}

				assert(stream.GetOffset() == location);
			}
		}
	}

	assert((location & (PACKAGE_MIN_ALIGN - 1)) == 0);
	assert(location == stream.GetOffset());
	// simulate reading
	lastOffset = 0;
	while (true) {
		if (location == maxLocation) {
			break;
		}

		if (!stream.Read(&header, sizeof(header))) {
			return false;
		}

		assert(header.seq != 0);
		assert(header.firstMark);
		if (header.seq > seq) {
			if (!stream.Seek(IStreamBase::CUR, -(long)sizeof(header))) {
				return false;
			}

			lastOffset = header.offset;
			break;
		}

		uint32_t length;
		if (!stream.Read(&length, sizeof(length))) {
			return false;
		}

		uint32_t alignedEnd = ((length + PACKAGE_MIN_ALIGN - 1) & ~(PACKAGE_MIN_ALIGN - 1));
		location += alignedEnd;
		if (!stream.Seek(IStreamBase::CUR, (long)alignedEnd - sizeof(length) - sizeof(header))) {
			return false;
		}

		assert(location == stream.GetOffset());
	}

	assert((location & (PACKAGE_MIN_ALIGN - 1)) == 0);
	return true;
}
#include "ZMemoryStream.h"
#include <string>
#include <ctime>

int ZPacket::main(int argc, char* argv[]) {
	const int BUFFER_SIZE = 4096*1024*10;
	ZMemoryStream file(BUFFER_SIZE), buffer(BUFFER_SIZE);
	ZPacket packet(file);

	String data[] = { "Hello, world!", "Here is an example of ZPacke", "Let's try", "ZPacket +++", "Very very very very very very very very very very very very very very very long text", "Your thoughts are mine!", "At last, we have revenged. Attacking!", "short" };
	size_t i, j, k, n;

	long totalLength = 0;
	for (i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
		buffer.Seek(IStreamBase::BEGIN, 0);
		size_t wl = data[i].size();
		buffer.Write(data[i].c_str(), wl);
		buffer.Seek(IStreamBase::BEGIN, 0);
		// printf("Seq : %u Total Length = 0x%02X\n", i, data[i].size() + sizeof(uint32_t));
		packet.WritePacket(i + 1, buffer, data[i].size());
		totalLength += data[i].size() + sizeof(int64_t) + sizeof(uint32_t);
	}


	for (i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
		buffer.Seek(IStreamBase::BEGIN, 0);
		size_t wl = data[i].size();
		buffer.Write(data[i].c_str(), wl);
		buffer.Seek(IStreamBase::BEGIN, 0);
		// printf("Seq : %d Total Length = 0x%02X\n", i + sizeof(data) / sizeof(data[0]), data[i].size() + sizeof(uint32_t));
		packet.WritePacket(i + 1 + sizeof(data) / sizeof(data[0]), buffer, data[i].size());
		totalLength += data[i].size() + sizeof(int64_t) + sizeof(uint32_t);
	}

	// read packets
//	packet.Seek(6);
	// packet.Seek(3);
	// packet.Seek(6);
//	packet.Seek(4);
//	packet.Seek(3);

	for (j = 0; j < sizeof(data) / sizeof(data[0]) * 2; j++) {
		buffer.Seek(IStreamBase::BEGIN, 0);
		int64_t seq;
		uint32_t size;
		if (packet.ReadPacket(seq, buffer, size)) {
			String tr((const char*)buffer.GetBuffer(), size);
			printf("%d packet : %s\n", (int)seq, tr.c_str());
		} else {
			printf("Invalid read\n");
		}
	}

	printf("-----------\n");
	// random data rushing
	const int MAX_LENGTH = 1024;
	char str[MAX_LENGTH] = "Test load";
	srand((unsigned int)time(nullptr));

	for (i = 0, n = 0; i < 10000; i++) {
		size_t length = 1 + (rand() % (MAX_LENGTH - 1));
		size_t wl = length;
		// int inc = rand() % 2;
		buffer.Seek(IStreamBase::BEGIN, 0);
		buffer.Write(str, wl);
		buffer.Seek(IStreamBase::BEGIN, 0);
		packet.WritePacket(n + 32, buffer, length);
		totalLength += length + sizeof(int64_t) + sizeof(uint32_t);
		if (i % 3 == 2) n++;
	}

	// printf("Usage: %.2f%% Padding: %.2f%%\n", (double)totalLength * 100 / packet.GetMaxLocation(), (double)packet.totalPadding * 100 / packet.GetMaxLocation());
	printf("Usage: %.2f%%\n", (double)totalLength * 100 / packet.GetMaxLocation());

	for (j = 0; j < 100; j++) {
		packet.Seek(rand() % (MAX_LENGTH + 32));
	}

	packet.Seek(37);

	for (k = 0; k < 10; k++) {
		buffer.Seek(IStreamBase::BEGIN, 0);
		int64_t seq;
		uint32_t size;
		if (packet.ReadPacket(seq, buffer, size)) {
			String tr((const char*)buffer.GetBuffer(), size);
			printf("%d packet : %s\n", (int)seq, tr.c_str());
		} else {
			printf("Invalid read\n");
		}
	}

	return 0;
}
*/
