#include "IImage.h"
#include "Interfaces.h"

using namespace PaintsNow;

int IImage::GetPixelSize(IRender::Resource::TextureDescription::Format dataType, IRender::Resource::TextureDescription::Layout layout) {
	int count = sizeof(float), size = 4;
	switch (dataType) {
		case IRender::Resource::TextureDescription::FLOAT: size = sizeof(float); break;
		case IRender::Resource::TextureDescription::HALF: size = sizeof(float) / 2; break;
		case IRender::Resource::TextureDescription::UNSIGNED_BYTE: size = sizeof(unsigned char); break;
		case IRender::Resource::TextureDescription::UNSIGNED_SHORT: size = sizeof(unsigned short); break;
	}
	switch (layout) {
		case IRender::Resource::TextureDescription::R: count = 1; break;
		case IRender::Resource::TextureDescription::RG: count = 2; break;
		case IRender::Resource::TextureDescription::RGB: count = 3; break;
		case IRender::Resource::TextureDescription::RGBA: count = 4; break;
	}

	return size * count;
}

IImage::~IImage() {}