// IRender.h - Basic render interface
// By PaintDream (paintdream@paintdream.com)
// 2019-9-19
//

#ifndef __IRENDER_H__
#define __IRENDER_H__

#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Template/TProxy.h"
#include "../../Core/Template/TBuffer.h"
#include "../../Core/Interface/IDevice.h"
#include <string>
#include <map>

namespace PaintsNow {
	class IShader;
	class IRender;
	class IRender : public IDevice {
	public:
		virtual ~IRender();
		class Device {};
		class Queue {};
		class Resource {
		public:
			enum Type {
				RESOURCE_UNKNOWN = 0,
				RESOURCE_TEXTURE,
				RESOURCE_BUFFER,
				RESOURCE_SHADER,
				RESOURCE_RENDERSTATE,
				RESOURCE_RENDERTARGET,
				RESOURCE_CLEAR,
				RESOURCE_DRAWCALL,
				RESOURCE_NOTIFY
			};

			struct Description {
				enum Format {
					UNSIGNED_BYTE, UNSIGNED_SHORT, 
					HALF_FLOAT,
					FLOAT,
					// Following are not available in texture
					UNSIGNED_INT
				};
			};

			struct BufferDescription : public Description {
				enum Usage {
					INDEX, VERTEX, INSTANCED, UNIFORM, SHARED
				};

				BufferDescription() : usage(VERTEX), format(FLOAT), dynamic(0), component(4) {}

				uint8_t usage;
				uint8_t format;
				uint16_t dynamic : 1;
				uint16_t component : 15;
				Bytes data;
			};

			struct TextureDescription : public Description {
				enum Dimension {
					TEXTURE_1D, TEXTURE_2D, TEXTURE_2D_CUBE, TEXTURE_3D
				};

				enum Layout {
					R, RG, RGB, RGBA, DEPTH, STENCIL, DEPTH_STENCIL, RGB10A2, RGB10F
				};

				enum Sample {
					POINT, LINEAR, ANSOTRIPIC, RENDERBUFFER // RenderBuffer can only be used as color attachment.
				};

				enum Mip {
					NOMIP, AUTOMIP, SPECMIP
				};

				TextureDescription() : dimension(0, 0, 0) {}
				
				struct State {
					State() : type(TEXTURE_2D), format(UNSIGNED_BYTE), 
						wrap(1), immutable(1), reserved(1), layout(RGBA), sample(LINEAR), mip(NOMIP), compress(0) {}

					inline bool operator == (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) == 0;
					}

					inline bool operator != (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) != 0;
					}
					
					inline bool operator < (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) < 0;
					}

					uint32_t type : 2;
					uint32_t format : 2;
					uint32_t wrap : 1;
					uint32_t immutable : 1;
					uint32_t reserved : 1;
					uint32_t compress : 1;
					uint32_t layout : 4;
					uint32_t sample : 2;
					uint32_t mip : 2;
				};

				Bytes data;
				UShort3 dimension;
				State state;
			};

			struct ShaderDescription : public Description {
				enum Stage {
					GLOBAL, VERTEX, TESSELLATION_CONTROL, TESSELLATION_EVALUATION, GEOMETRY, FRAGMENT, COMPUTE, END
				};

				String name;
				TWrapper<void, ShaderDescription&, Stage, const String&, const String&> compileCallback;
				std::vector<std::pair<Stage, IShader* > > entries;
			};

			// Executable
			struct RenderStateDescription : public Description {
				RenderStateDescription() { memset(this, 0, sizeof(*this)); }
				enum Test {
					DISABLED, ALWAYS, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, EQUAL
				};

				inline bool operator < (const RenderStateDescription& rhs) const {
					return memcmp(this, &rhs, sizeof(*this)) < 0;
				}

				inline bool operator == (const RenderStateDescription& rhs) const {
					return memcmp(this, &rhs, sizeof(*this)) == 0;
				}

				inline bool operator != (const RenderStateDescription& rhs) const {
					return memcmp(this, &rhs, sizeof(*this)) != 0;
				}

				uint32_t stencilReplacePass : 1;
				uint32_t stencilReplaceFail : 1;
				uint32_t stencilReplaceZFail : 1;
				uint32_t cullFrontFace : 1;
				uint32_t cull : 1;
				uint32_t fill : 1;
				uint32_t alphaBlend : 1;
				uint32_t colorWrite : 1;

				uint32_t depthTest : 3;
				uint32_t depthWrite : 1;
				uint32_t stencilTest : 3;
				uint32_t stencilWrite : 1;
				uint32_t stencilMask : 8;
				uint32_t stencilValue : 8;
			};

			struct RenderTargetDescription : public Description {
				RenderTargetDescription() : range(UShort2(0, 0), UShort2(0, 0)){}

				// attachments
				UShort2Pair range;

				struct Storage {
					Storage() : mipLevel(0), resource(nullptr) {}
					uint32_t mipLevel; // used for render to mip
					Resource* resource;
				};

				Storage depthStencilStorage;
				std::vector<Storage> colorBufferStorages; // 0 for backbuffer
			};

			struct ClearDescription : public Description {
				enum {
					CLEAR = 1 << 0,
					DISCARD_LOAD = 1 << 1,
					DISCARD_STORE = 1 << 2
				};

				ClearDescription() : clearColor(0, 0, 0, 0) {
					clearColorBit = clearDepthBit = clearStencilBit = 0;
				}

				uint32_t clearColorBit : 4;
				uint32_t clearDepthBit : 4;
				uint32_t clearStencilBit : 4;
				uint32_t reserved : 4;
				Float4 clearColor;
			};
			
			struct DrawCallDescription : public Description {
				DrawCallDescription() : shaderResource(nullptr), instanceCounts(0, 0, 0) {}
				Resource* shaderResource;
				UInt3 instanceCounts; // y/z for compute shaders
				
				struct BufferRange {
					BufferRange() : buffer(nullptr), offset(0), length(0), component(0) {}

					Resource* buffer;
					uint32_t offset;
					uint32_t length;
					uint32_t component;
				};

				BufferRange indexBufferResource;
				std::vector<BufferRange> bufferResources;
				std::vector<Resource*> textureResources;
			};

			struct NotifyDescription : public Description {
				TWrapper<void, IRender&, Queue*> notifier;
			};
		};

		// The only API that requires calling on device thread.
		enum PresentOption {
			PRESENT_EXECUTE_ALL,
			PRESENT_REPEAT,
			PRESENT_EXECUTE,
			PRESENT_CONSUME,
			PRESENT_CLEAR_ALL
		};

		virtual void PresentQueues(Queue** queues, uint32_t count, PresentOption option) = 0;

		// Device
		virtual Device* CreateDevice(const String& description) = 0;
		virtual bool SupportParallelPresent(Device* device) = 0;
		virtual Int2 GetDeviceResolution(Device* device) = 0;
		virtual void SetDeviceResolution(Device* device, const Int2& resolution) = 0;
		virtual void DeleteDevice(Device* device) = 0;

		// Queue
		enum QueueFlag {
			QUEUE_MULTITHREAD
		};

		virtual Queue* CreateQueue(Device* device, uint32_t flag = 0) = 0;
		virtual Device* GetQueueDevice(Queue* queue) = 0;
		virtual void DeleteQueue(Queue* queue) = 0;
		virtual void MergeQueue(Queue* target, Queue* source) = 0;
		virtual void FlushQueue(Queue* queue) = 0;
		virtual bool IsQueueEmpty(Queue* queue) = 0;

		// Resource
		virtual Resource* CreateResource(Device* device, Resource::Type resourceType) = 0;
		virtual void DeleteResource(Queue* queue, Resource* resource) = 0; // must delete resource on a queue
		virtual void UploadResource(Queue* queue, Resource* resource, Resource::Description* description) = 0;
		virtual void AcquireResource(Queue* queue, Resource* resource) = 0;
		virtual void ReleaseResource(Queue* queue, Resource* resource) = 0;
		virtual void RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) = 0;
		virtual void CompleteDownloadResource(Queue* queue, Resource* resource) = 0;
		virtual void ExecuteResource(Queue* queue, Resource* resource) = 0;
		virtual void SwapResource(Queue* queue, Resource* lhs, Resource* rhs) = 0;
	};
}

#endif // __IRENDER_H__
