// IRender.h - Basic render interface
// PaintDream (paintdream@paintdream.com)
// 2019-9-19
//

#pragma once
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
	class pure_interface IRender : public IDevice {
	public:
		~IRender() override;
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
				RESOURCE_DRAWCALL,
			};

			struct Description {
				enum Format {
					UNSIGNED_BYTE,
					UNSIGNED_SHORT, 
					HALF,
					FLOAT,
					// Following are not available in texture
					UNSIGNED_INT,
					END
				};
			};

			struct BufferDescription : public Description {
				enum Usage {
					INDEX, VERTEX, INSTANCED, CONSTANT, UNIFORM, STORAGE
				};

				BufferDescription() : usage(VERTEX), format(FLOAT), dynamic(0), component(4), stride(0), offset(0), length(0) {}

				uint32_t usage : 4;
				uint32_t format : 3;
				uint32_t dynamic : 1;
				uint8_t component;
				uint16_t stride;
				uint32_t offset;
				uint32_t length;

				Bytes data;
			};

			struct TextureDescription : public Description {
				enum Dimension {
					TEXTURE_1D, TEXTURE_2D, TEXTURE_2D_CUBE, TEXTURE_3D
				};

				enum Layout {
					R, RG, RGB, RGBA, DEPTH, STENCIL, DEPTH_STENCIL, RGB10PACK, END
				};

				enum Sample {
					POINT, LINEAR, ANSOTRIPIC
				};

				enum Media {
					TEXTURE_RESOURCE,
					RENDERBUFFER, // RenderBuffer can only be used as color/depth/stencil attachment and render buffer fetch.
					RENDERBUFFER_FETCH,
				};

				enum Mip {
					NOMIP, AUTOMIP, SPECMIP
				};

				enum Compress {
					NONE, BPTC, ASTC, DXT
				};

				enum Address {
					REPEAT, CLAMP, MIRROR_REPEAT, MIRROR_CLAMP
				};

				enum Block {
					BLOCK_4X4,
					BLOCK_5X4,
					BLOCK_5X5,
					BLOCK_6X5,
					BLOCK_6X6,
					BLOCK_8X5,
					BLOCK_8X6,
					BLOCK_10X5,
					BLOCK_10X6,
					BLOCK_8X8,
					BLOCK_10X8,
					BLOCK_10X10,
					BLOCK_12X10,
					BLOCK_12X12,
				};

				TextureDescription() : dimension(0, 0, 0), frameBarrierIndex(0) {}
				
				struct State {
					State() : type(TEXTURE_2D), format(UNSIGNED_BYTE), sample(LINEAR), 
						layout(RGBA), addressU(REPEAT), addressV(REPEAT), addressW(REPEAT),
						mip(NOMIP), media(TEXTURE_RESOURCE), immutable(1), attachment(0), pcf(0), 
						compress(NONE), block(BLOCK_4X4) {}

					inline bool operator == (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) == 0;
					}

					inline bool operator != (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) != 0;
					}
					
					inline bool operator < (const State& rhs) const {
						return memcmp(this, &rhs, sizeof(*this)) < 0;
					}

					uint32_t type : 3;
					uint32_t format : 3;
					uint32_t sample : 2;
					uint32_t layout : 4;
					uint32_t addressU : 2;
					uint32_t addressV : 2;
					uint32_t addressW : 2;
					uint32_t mip : 2;
					uint32_t media : 2;
					uint32_t immutable : 1;
					uint32_t attachment : 1;
					uint32_t pcf : 1;
					uint32_t compress : 3;
					uint32_t block : 4;
				};

				Bytes data;
				UShort3 dimension; // width, height, depth
				uint16_t frameBarrierIndex;
				State state;
			};

			struct ShaderDescription : public Description {
				enum Stage {
					GLOBAL, VERTEX, TESSELLATION_CONTROL, TESSELLATION_EVALUATION, GEOMETRY, FRAGMENT, COMPUTE, END
				};

				String name;
				void* context;
				TWrapper<void, Resource*, ShaderDescription&, Stage, const String&, const String&> compileCallback;
				std::vector<std::pair<Stage, IShader* > > entries;
			};

			// Executable
			struct RenderStateDescription : public Description {
				RenderStateDescription() { memset(this, 0, sizeof(*this)); }
				enum Test {
					DISABLED, NEVER, LESS, EQUAL, LESS_EQUAL, GREATER, GREATER_EQUAL, ALWAYS
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
				uint32_t blend : 1;
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

				enum {
					DEFAULT,
					DISCARD,
					CLEAR,
					AUTO
				};

				struct Storage {
					Storage() : loadOp(DEFAULT), storeOp(DEFAULT), mipLevel(0), backBuffer(0), resource(nullptr), clearColor(0, 0, 0, 0) {}

					uint8_t loadOp;
					uint8_t storeOp;
					uint8_t mipLevel; // used for render to mip
					uint8_t backBuffer;
					Resource* resource;
					Float4 clearColor;
				};

				Storage depthStorage;
				Storage stencilStorage;
				std::vector<Storage> colorStorages; // 0 for backbuffer
				std::vector<Resource*> depResources;
			};
			
			struct DrawCallDescription : public Description {
				DrawCallDescription() : shaderResource(nullptr), instanceCounts(0, 0, 0), bufferCount(0), textureCount(0) {
					memset(textureResources, 0, sizeof(textureResources));
				}

				Resource* shaderResource;
				UInt3 instanceCounts; // y/z for compute shaders
				uint16_t bufferCount;
				uint16_t textureCount;
				
				struct BufferRange {
					BufferRange() : buffer(nullptr), offset(0), length(0), component(0), type(0) {}

					Resource* buffer;
					uint32_t offset;
					uint32_t length;
					uint16_t component;
					uint16_t type;
				};

				BufferRange indexBufferResource;
				BufferRange bufferResources[8]; // to avoid dynamic memory allocation
				Resource* textureResources[16];
				std::vector<BufferRange> extraBufferResources;
				std::vector<Resource*> extraTextureResources;
			};
		};

		// The only API that requires calling on device thread.
		enum PresentOption {
			PRESENT_EXECUTE_ALL,
			PRESENT_EXECUTE,
			PRESENT_REPEAT,
			PRESENT_CLEAR_ALL
		};

		virtual void PresentQueues(Queue** queues, uint32_t count, PresentOption option) = 0;

		// Device
		virtual std::vector<String> EnumerateDevices() = 0;
		virtual Device* CreateDevice(const String& description) = 0;
		virtual size_t GetProfile(Device* device, const String& feature) = 0;
		virtual Int2 GetDeviceResolution(Device* device) = 0;
		virtual void SetDeviceResolution(Device* device, const Int2& resolution) = 0;
		virtual void NextDeviceFrame(Device* device) = 0;
		virtual void DeleteDevice(Device* device) = 0;

		// Queue
		enum QueueFlag {
			QUEUE_REPEATABLE = 1 << 0,
			QUEUE_MULTITHREAD = 1 << 1
		};

		virtual Queue* CreateQueue(Device* device, uint32_t flag = 0) = 0;
		virtual Device* GetQueueDevice(Queue* queue) = 0;
		virtual void DeleteQueue(Queue* queue) = 0;
		virtual void FlushQueue(Queue* queue) = 0;

		// Resource
		virtual Resource* CreateResource(Device* device, Resource::Type resourceType) = 0;
		virtual void DeleteResource(Queue* queue, Resource* resource) = 0; // must delete resource on a queue
		virtual void UploadResource(Queue* queue, Resource* resource, Resource::Description* description) = 0;
		virtual void AcquireResource(Queue* queue, Resource* resource) = 0;
		virtual void ReleaseResource(Queue* queue, Resource* resource) = 0;
		virtual void RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) = 0;
		virtual void CompleteDownloadResource(Queue* queue, Resource* resource) = 0;
		virtual void ExecuteResource(Queue* queue, Resource* resource) = 0;
		virtual void SetResourceNotation(Resource* lhs, const String& note) = 0;
	};
}

