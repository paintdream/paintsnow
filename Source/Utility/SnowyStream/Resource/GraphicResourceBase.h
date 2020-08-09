// GraphicResourceBase.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-11
//

#ifndef __GRAPHICRESOURCEBASE_H__
#define __GRAPHICRESOURCEBASE_H__

#include "../ResourceBase.h"
#include "../../../General/Interface/IRender.h"
#include "../../../General/Misc/PassBase.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class IDataUpdater {
		public:
			virtual void Update(IRender& render, IRender::Queue* queue) = 0;
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

		class GraphicResourceBase : public TReflected<GraphicResourceBase, DeviceResourceBase<IRender> > {
		public:
			static inline void ClearBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer) {
				if (buffer != nullptr) {
					render.DeleteResource(queue, buffer);
					buffer = nullptr;
				}
			}

			template <class T>
			static inline size_t UpdateBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer, std::vector<T>& data, IRender::Resource::BufferDescription::Usage usage, uint32_t groupSize = 1) {
				size_t size = data.size();
				if (!data.empty()) {
					IRender::Resource::BufferDescription description;
					description.data.Resize(safe_cast<uint32_t>(data.size() * sizeof(T)));
					description.usage = usage;
#if defined(_MSC_VER) && _MSC_VER <= 1200
					description.component = sizeof(T) / sizeof(T::type) * groupSize;
					description.format = MapFormat<T::type>::format;
#else
					description.component = sizeof(T) / sizeof(typename T::type) * groupSize;
					description.format = MapFormat<typename T::type>::format;
#endif
					memcpy(description.data.GetData(), &data[0], data.size() * sizeof(T));

					if (buffer == nullptr) {
						buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
					}

					render.UploadResource(queue, buffer, &description);
				}

				return size;
			}

			GraphicResourceBase(ResourceManager& manager, const String& uniqueID);
			virtual ~GraphicResourceBase();
			virtual void Attach(IRender& device, void* deviceContext) override;
			virtual void Detach(IRender& device, void* deviceContext) override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		};
	}
}


#endif // __GRAPHICRESOURCEBASE_H__