// RenderResourceBase.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-11
//

#pragma once
#include "../ResourceBase.h"
#include "../../../General/Interface/IRender.h"
#include "../../../General/Misc/PassBase.h"

namespace PaintsNow {
	class IDataUpdater {
	public:
		virtual uint32_t Update(IRender& render, IRender::Queue* queue) = 0;
	};

	template <class T>
	struct MapFormat {};

	template <>
	struct MapFormat<float> {
		enum { format = IRender::Resource::BufferDescription::FLOAT };
	};

	template <>
	struct MapFormat<uint8_t> {
		enum { format = IRender::Resource::BufferDescription::UNSIGNED_BYTE };
	};

	template <>
	struct MapFormat<uint16_t> {
		enum { format = IRender::Resource::BufferDescription::UNSIGNED_SHORT };
	};

	template <>
	struct MapFormat<uint32_t> {
		enum { format = IRender::Resource::BufferDescription::UNSIGNED_INT };
	};

	class RenderResourceBase : public TReflected<RenderResourceBase, DeviceResourceBase<IRender> > {
	public:
		static inline void ClearBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer) {
			if (buffer != nullptr) {
				render.DeleteResource(queue, buffer);
				buffer = nullptr;
			}
		}

		template <class T>
		inline uint32_t UpdateBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer, std::vector<T>& data, IRender::Resource::BufferDescription::Usage usage, const char* note, uint32_t groupSize = 1) {
			uint32_t size = verify_cast<uint32_t>(data.size());
			if (!data.empty()) {
				IRender::Resource::BufferDescription description;
				description.data.Resize(verify_cast<uint32_t>(data.size() * sizeof(T)));
				description.usage = usage;
#if defined(_MSC_VER) && _MSC_VER <= 1200
				description.component = verify_cast<uint8_t>(sizeof(T) / sizeof(T::type) * groupSize);
				description.format = MapFormat<T::type>::format;
#else
				description.component = verify_cast<uint8_t>(sizeof(T) / sizeof(typename T::type) * groupSize);
				description.format = MapFormat<typename T::type>::format;
#endif
				description.stride = verify_cast<uint16_t>(sizeof(T) * groupSize);
				memcpy(description.data.GetData(), &data[0], data.size() * sizeof(T));

				if (buffer == nullptr) {
					buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
				}

#ifdef _DEBUG
				render.SetResourceNotation(buffer, GetLocation() + "@" + note);
#endif
				render.UploadResource(queue, buffer, &description);
			}

			return size;
		}

		RenderResourceBase(ResourceManager& manager, const String& uniqueID);
		~RenderResourceBase() override;
		bool Complete(size_t version) override;
		void Refresh(IRender& device, void* deviceContext) override;
		void Attach(IRender& device, void* deviceContext) override;
		void Detach(IRender& device, void* deviceContext) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		std::atomic<size_t> runtimeVersion; // for resource updating synchronization
	};
}

