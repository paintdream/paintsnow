#include "TextureResource.h"
#include "../ResourceManager.h"
#include "../../../General/Interface/IImage.h"
#include "../../../Core/System/MemoryStream.h"

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_BPTC_BUILTIN
#include "../../../General/Driver/Filter/BPTC/ZFilterBPTC.h"
#endif

using namespace PaintsNow;

TextureResource::TextureResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), instance(nullptr), deviceMemoryUsage(0) {
	description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

void TextureResource::Attach(IRender& render, void* deviceContext) {
	GraphicResourceBase::Attach(render, deviceContext);
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	assert(queue != nullptr);
	assert(instance == nullptr);
	instance = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_TEXTURE);
}

void TextureResource::Detach(IRender& render, void* deviceContext) {
	if (instance != nullptr) {
		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
		render.DeleteResource(queue, instance);
		instance = nullptr;
	}

	GraphicResourceBase::Detach(render, deviceContext);
}

void TextureResource::Unmap() {
	GraphicResourceBase::Unmap();
	
	SpinLock(critical);
	if (mapCount.load(std::memory_order_relaxed) == 0) {
		description.data.Clear();
	}
	SpinUnLock(critical);
}

void TextureResource::Upload(IRender& render, void* deviceContext) {
	// if (description.data.size() == 0) return;
	//	assert(description.data.size() == (size_t)description.dimension.x() * description.dimension.y() * IImage::GetPixelSize((IRender::Resource::TextureDescription::Format)description.state.format, (IRender::Resource::TextureDescription::Layout)description.state.layout));
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);

	if (!(Flag().fetch_or(RESOURCE_UPLOADED) & RESOURCE_UPLOADED)) {
		SpinLock(critical);

		description.state.reserved = 0;
		description.state.media = 0;
		if (description.state.compress) {
			assert(!description.data.Empty());
		}

		deviceMemoryUsage = description.data.GetSize();
		if (mapCount.load(std::memory_order_relaxed) != 0) {
			IRender::Resource::TextureDescription desc = description;
			render.UploadResource(queue, instance, &desc);
		} else {
			render.UploadResource(queue, instance, &description);
		}

#ifdef _DEBUG
		render.SetResourceNotation(instance, GetLocation());
#endif

		SpinUnLock(critical);

		Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	}
}

void TextureResource::Download(IRender& render, void* deviceContext) {
	// data.resize((size_t)dimension.x() * dimension.y() * IImage::GetPixelSize((IRender::Resource::TextureDescription::Format)state.format, (IRender::Resource::TextureDescription::Layout)state.layout));
	/*
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	render.RequestDownloadResource(queue, instance, this);*/
}

IRender::Resource* TextureResource::GetRenderResource() const {
	return instance;
}

TObject<IReflect>& TextureResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		// serializable
		IRender::Resource::TextureDescription::State& state = description.state;
		UShort3& dimension = description.dimension;
		Bytes& data = description.data;

		ReflectProperty(state);
		ReflectProperty(dimension);
		ReflectProperty(data);

		// runtime
		// ReflectProperty(instance)[Runtime];
	}

	return *this;
}

size_t TextureResource::ReportDeviceMemoryUsage() const {
	return deviceMemoryUsage;
}

bool TextureResource::Compress(const String& compressionType) {
#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_BPTC_BUILTIN
	if (compressionType == "BPTC") { // BC7
		static ZFilterBPTC factory;
		const UShort3& dimension = description.dimension;
		uint32_t w = dimension.x(), h = dimension.y();
		if (w < 4 || w != h || description.data.Empty() || description.state.compress) return false;
		if (description.state.format != IRender::Resource::TextureDescription::UNSIGNED_BYTE) return false;

		MemoryStream src(sizeof(UChar4) * w * h, 128);
		if (description.state.layout == IRender::Resource::TextureDescription::RGBA) {
			memcpy(src.GetBuffer(), description.data.GetData(), description.data.GetSize());
		} else {
			// padding to RGBA8
			char* buf = (char*)src.GetBuffer();
			memset(buf, 0xFF, src.GetTotalLength());
			const uint8_t* p = description.data.GetData();
			uint32_t dim = description.state.layout;
			for (uint32_t i = 0; i < w * h; i++) {
				for (uint32_t k = 0; k < dim; k++) {
					buf[i * 4 + k] = *p++;
				}
			}
		}

		// get mip count
		uint32_t mipCount = 1;

		if (description.state.mip != IRender::Resource::TextureDescription::NOMIP) {
			description.state.mip = IRender::Resource::TextureDescription::SPECMIP;
			mipCount = Math::Log2((uint32_t)dimension.x()) - 2;
		}

		size_t length = 0;
		for (uint32_t i = 0; i < mipCount; i++) {
			length += w * h;
			w >>= 1;
			h >>= 1;
		}

		MemoryStream target(length, 128);
		IStreamBase* filter = factory.CreateFilter(target);

		w = dimension.x();
		h = dimension.y();

		for (uint32_t k = 0; k < mipCount; k++) {
			size_t len = w * h * sizeof(UChar4);
			filter->Write(src.GetBuffer(), len);

			if (k != mipCount - 1) {
				// generate mip data
				UChar4* p = reinterpret_cast<UChar4*>(src.GetBuffer());
				for (uint32_t y = 0; y < h / 2; y++) {
					for (uint32_t x = 0; x < w / 2; x++) {
						UShort4 result;
						for (uint32_t yy = 0; yy < 2; yy++) {
							for (uint32_t xx = 0; xx < 2; xx++) {
								const UChar4& v = p[(y * 2 + yy) * w + (x * 2 + xx)];
								for (uint32_t m = 0; m < 4; m++) {
									result[m] += v[m];
								}
							}
						}

						p[y * w / 2 + x] = UChar4(result[0] >> 2, result[1] >> 2, result[2] >> 2, result[3] >> 2);
					}
				}
			}

			w >>= 1;
			h >>= 1;
		}

		// TODO: conflicts with mapped resource
		assert(mapCount.load(std::memory_order_relaxed) == 0);
		SpinLock(critical);
		description.data.Assign((uint8_t*)target.GetBuffer(), safe_cast<uint32_t>(target.GetTotalLength()));
		description.state.compress = 1;
		description.state.layout = IRender::Resource::TextureDescription::RGBA;
		SpinUnLock(critical);

		filter->ReleaseObject();
		return true;
	} else {
		return false;
	}
#else
	return false;
#endif
}

bool TextureResource::LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length) {
	IImage& imageBase = interfaces.image;
	IImage::Image* image = imageBase.Create(1, 1, IRender::Resource::TextureDescription::RGB, IRender::Resource::TextureDescription::Format::UNSIGNED_BYTE);
	if (image == nullptr) return false;
	bool success = imageBase.Load(image, streamBase, length);
	IRender::Resource::TextureDescription::Layout layout = imageBase.GetLayoutType(image);
	IRender::Resource::TextureDescription::Format dataType = imageBase.GetDataType(image);

	SpinLock(critical);
	description.state.layout = layout;
	description.state.format = dataType;
	description.dimension.x() = safe_cast<uint16_t>(imageBase.GetWidth(image));
	description.dimension.y() = safe_cast<uint16_t>(imageBase.GetHeight(image));

	void* buffer = imageBase.GetBuffer(image);
	description.data.Assign(reinterpret_cast<uint8_t*>(buffer), (size_t)description.dimension.x() * description.dimension.y() * IImage::GetPixelSize(dataType, layout));
	SpinUnLock(critical);

	// copy info
	imageBase.Delete(image);
	return success;
}